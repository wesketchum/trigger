/** TriggerZipper is an appfwk::module that runs zipper::merge
 */

#ifndef TRIGGER_PLUGINS_TRIGGERZIPPER_HPP_
#define TRIGGER_PLUGINS_TRIGGERZIPPER_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "trigger/triggerzipper/Nljs.hpp"
#include "zipper.hpp"

#include <list>
#include <chrono>

const char* inqs_name = "inputs";
const char* outq_name = "output";

namespace dunedaq::trigger {

// template<typename Payload,
//          typename Ordering = size_t,
//          typename Identity = size_t,
//          typename TimePoint = std::chrono::steady_clock::time_point,
//          typename 
// >

size_t zipper_stream_id(const dataformats::GeoID& geoid) {
    return
        (0xffff000000000000 & (static_cast<size_t>(geoid.system_type) << 48)) |
        (0x0000ffff00000000 & (static_cast<size_t>(geoid.region_id) << 32)) |
        (0x00000000ffffffff & geoid.element_id);
}
    
template<typename TSET>
class TriggerZipper : public dunedaq::appfwk::DAQModule {

  public:
    // Derived types
    using tset_type = TSET;
    using ordering_type = typename TSET::timestamp_t;
    using origin_type = typename TSET::origin_t; // GeoID
    using seqno_type = typename TSET::seqno_t; // GeoID

    using cache_type = std::list<TSET>;
    using payload_type = typename cache_type::iterator;
    using identity_type = size_t;

    using node_type = zipper::Node<payload_type>;
    using zm_type = zipper::merge<node_type>;
    zm_type m_zm;

    // queues
    using source_t = appfwk::DAQSource<TSET>;
    using sink_t = appfwk::DAQSink<TSET>;
    std::unique_ptr<source_t>  m_inq{};
    std::unique_ptr< sink_t > m_outq{};

    using cfg_t = triggerzipper::ConfParams;
    cfg_t m_cfg;

    std::thread m_thread;
    std::atomic<bool> m_running{false};

    // We store input TSETs in a list and send iterator though the
    // zipper as payload so as to not suffer copy overhead.
    cache_type cache;
    seqno_type m_next_seqno{0};

    explicit TriggerZipper(const std::string& name)
        : DAQModule(name), m_zm()
    {
        // clang-format off
        register_command("conf",   &TriggerZipper<TSET>::do_configure);
        register_command("start",  &TriggerZipper<TSET>::do_start);
        register_command("stop",   &TriggerZipper<TSET>::do_stop);
        register_command("pause",  &TriggerZipper<TSET>::do_pause);
        register_command("resume", &TriggerZipper<TSET>::do_resume);
        register_command("scrap",  &TriggerZipper<TSET>::do_scrap);
        // clang-format on
    }
    virtual ~TriggerZipper()
    {
    }

    void init(const nlohmann::json& ini)
    {
        set_input(appfwk::queue_inst(ini, "input"));
        set_output(appfwk::queue_inst(ini, "output"));
    }
    void set_input(const std::string& name)
    {
        m_inq.reset(new source_t(name));
    }
    void set_output(const std::string& name)
    {
        m_outq.reset(new sink_t(name));
    }

    void do_configure(const nlohmann::json& cfgobj)
    {
        m_cfg = cfgobj.get<cfg_t>();
        m_zm.set_cardinality(m_cfg.cardinality);
    }

    void do_scrap(const nlohmann::json& /*stopobj*/)
    {
        m_cfg = cfg_t{};
        m_zm.set_cardinality(0);
    }
        
    void do_start(const nlohmann::json& /*startobj*/)
    {
        m_running.store(true);
        m_thread = std::thread(&TriggerZipper::worker, this);
    }

    void do_stop(const nlohmann::json& /*stopobj*/)
    {
        m_running.store(false);
        m_thread.join();
        flush();
    }

    void do_pause(const nlohmann::json& /*pauseobj*/)
    {
        // fixme: need a 2nd flag?
        m_running.store(false);
    }

    void do_resume(const nlohmann::json& /*resumeobj*/)
    {
        m_running.store(true);
    }

    // thread worker
    void worker() {
        while (m_running.load()) {
            proc_one();
        }
    }

    void proc_one()
    {
        cache.emplace_front();  // to be filled
        auto& tset = cache.front();
        try {
            m_inq->pop(tset, std::chrono::milliseconds(10));
        }
        catch (appfwk::QueueTimeoutExpired&) {
            cache.pop_front(); // vestigial 
            drain();
            return;
        }

        bool accepted = m_zm.feed(cache.begin(),tset.start_time,
                                  zipper_stream_id(tset.origin));
        if (!accepted) {
            cache.pop_front(); // vestigial
        }
        drain();
    }

    void send_out(std::vector<node_type>& got)
    {
        for (auto& node : got) {
            payload_type lit = node.payload;
            auto& tset = *lit;  // list iterator

            // tell consumer "where" the set was produced
            tset.origin.region_id = m_cfg.region_id;
            tset.origin.element_id = m_cfg.element_id;
            tset.seqno = m_next_seqno++;

            try {
                m_outq->push(tset, std::chrono::milliseconds(10));
            }
            catch (const dunedaq::appfwk::QueueTimeoutExpired& err) {
                // our output queue is stuffed.  should more be done
                // here than simply complain and drop?
                ers::error(err);
            }
            cache.erase(lit);
        }
    }

    // Maybe drain and send to out queue
    void drain()
    {
        std::vector<node_type> got;
        if (m_cfg.max_latency_ms) {
            m_zm.drain_prompt(std::back_inserter(got));
        }
        else {
            m_zm.drain_waiting(std::back_inserter(got));
        }
        send_out(got);
    }

    // Fully drain and send to out queue
    void flush()
    {
        std::vector<node_type> got;
        m_zm.drain_full(std::back_inserter(got));
        send_out(got);
    }

};
}

/// Need one of these in a .cpp for each concrete TSET type
//DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TriggerZipper<TSET>)

#endif
// Local Variables:
// c-basic-offset: 4
// End:
