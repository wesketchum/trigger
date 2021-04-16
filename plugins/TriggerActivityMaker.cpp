#include "TriggerActivityMaker.hpp"

#include "appfwk/DAQModuleHelper.hpp"

#include "trigger/triggeractivitymaker/Nljs.hpp"
#include "trigger/Plugins.hpp"

namespace dunedaq::trigger {

TriggerActivityMaker::TriggerActivityMaker(const std::string& name):
  DAQModule(name),
  m_thread(std::bind(&TriggerActivityMaker::do_work, this, std::placeholders::_1)),
  m_input_queue(nullptr),
  m_output_queue(nullptr),
  m_queue_timeout(100)
{
  register_command("start"    , &TriggerActivityMaker::do_start    );
  register_command("stop"     , &TriggerActivityMaker::do_stop     );
  register_command("conf", &TriggerActivityMaker::do_configure);
}

void TriggerActivityMaker::init(const nlohmann::json& iniobj) {
  m_input_queue.reset(new source_t(appfwk::queue_inst(iniobj, "input")));
  m_output_queue.reset(new sink_t(appfwk::queue_inst(iniobj, "output")));
}

void TriggerActivityMaker::do_start(const nlohmann::json& /*args*/) {
  m_thread.start_working_thread();
}

void TriggerActivityMaker::do_stop(const nlohmann::json& /*args*/) {
  m_thread.stop_working_thread();
}

void TriggerActivityMaker::do_configure(const nlohmann::json& config) {
  auto params = config.get<triggeractivitymaker::Conf>();
  m_activity_maker=make_ta_maker(params.activity_maker);
}

void TriggerActivityMaker::do_work(std::atomic<bool>& running_flag) {
  int received_count = 0;
  int sent_count = 0;

  while (running_flag.load()) {
    triggeralgs::TriggerPrimitive prim;
    try {
      m_input_queue->pop(prim, m_queue_timeout);
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      // it is perfectly reasonable that there might be no data in the queue 
      // some fraction of the times that we check, so we just continue on and try again
      continue;
    }
    ++received_count;

    std::vector<triggeralgs::TriggerActivity> tas;
    m_activity_maker->operator()(prim, tas);
        
    while(tas.size()) {
      bool successfullyWasSent = false;
      while (!successfullyWasSent && running_flag.load()) {
        try {
          m_output_queue->push(tas.back(), m_queue_timeout);
          tas.pop_back();
          successfullyWasSent = true;
          ++sent_count;
        } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
          ers::warning(excpt);
        }
      }
    }
  } // end while (running_flag.load()) 

  TLOG() << ": Exiting do_work() method, received " << received_count
         << " TPs and successfully sent " << sent_count << " TAs. ";
}


} // namespace dunedaq::trigger


DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TriggerActivityMaker)
