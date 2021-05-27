#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "logging/Logging.hpp"

#include <memory>
#include <string>

namespace dunedaq::trigger {

template<class IN, class OUT, class MAKER>
class TriggerGenericMaker : public dunedaq::appfwk::DAQModule
{
public:
  explicit TriggerGenericMaker(const std::string& name)
    : DAQModule(name)
    , m_thread(std::bind(&TriggerGenericMaker::do_work, this, std::placeholders::_1))
    , m_input_queue(nullptr)
    , m_output_queue(nullptr)
    , m_queue_timeout(100)
  {
    register_command("start", &TriggerGenericMaker::do_start);
    register_command("stop", &TriggerGenericMaker::do_stop);
    register_command("conf", &TriggerGenericMaker::do_configure);
  }
  
  virtual ~TriggerGenericMaker() { }

  TriggerGenericMaker(const TriggerGenericMaker&) = delete;
  TriggerGenericMaker& operator=(const TriggerGenericMaker&) = delete;
  TriggerGenericMaker(TriggerGenericMaker&&) = delete;
  TriggerGenericMaker& operator=(TriggerGenericMaker&&) = delete;

  void init(const nlohmann::json& obj) override
  {
    m_input_queue.reset(new source_t(appfwk::queue_inst(obj, "input")));
    m_output_queue.reset(new sink_t(appfwk::queue_inst(obj, "output")));
  }

private:
  dunedaq::appfwk::ThreadHelper m_thread;
  
  using source_t = dunedaq::appfwk::DAQSource<IN>;
  std::unique_ptr<source_t> m_input_queue;

  using sink_t = dunedaq::appfwk::DAQSink<OUT>;
  std::unique_ptr<sink_t> m_output_queue;

  std::chrono::milliseconds m_queue_timeout;

  std::shared_ptr<MAKER> m_maker;
  
  virtual std::shared_ptr<MAKER> make_maker(const nlohmann::json& obj) = 0;
  
  void do_start(const nlohmann::json& /*obj*/)
  {
    m_thread.start_working_thread();
  }
  
  void do_stop(const nlohmann::json& /*obj*/)
  {
    m_thread.stop_working_thread();
  }
  
  void do_configure(const nlohmann::json& obj)
  {
    m_maker = make_maker(obj);
  }
  
  void do_work(std::atomic<bool> &running_flag)
  {
    int received_count = 0;
    int sent_count = 0;

    while (running_flag.load()) {
      IN in;
      try {
        m_input_queue->pop(in, m_queue_timeout);
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        // it is perfectly reasonable that there might be no data in the queue
        // some fraction of the times that we check, so we just continue on and try again
        continue;
      }
      ++received_count;

      std::vector<OUT> outs;
      m_maker->operator()(in, outs);

      while (outs.size()) {
        bool successfullyWasSent = false;
        while (!successfullyWasSent && running_flag.load()) {
          try {
            m_output_queue->push(outs.back(), m_queue_timeout);
            outs.pop_back();
            successfullyWasSent = true;
            ++sent_count;
          } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
            ers::warning(excpt);
          }
        }
      }
    } // end while (running_flag.load())

    TLOG() << ": Exiting do_work() method, received " << received_count << " inputs and successfully sent " << sent_count
           << " outputs. ";
  }

};

} // namespace dunedaq::trigger
