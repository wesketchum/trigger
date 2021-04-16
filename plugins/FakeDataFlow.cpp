/**
 * @file FakeDataFlow.cpp FakeDataFlow class implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "FakeDataFlow.hpp"

#include "dataformats/ComponentRequest.hpp"

#include "dfmessages/TriggerDecision.hpp"
#include "dfmessages/TriggerDecisionToken.hpp"
#include "dfmessages/Types.hpp"
#include "logging/Logging.hpp"

#include "trigger/fakedataflow/Nljs.hpp"

#include "appfwk/app/Nljs.hpp"

namespace dunedaq {
namespace trigger {

FakeDataFlow::FakeDataFlow(const std::string& name)
  : DAQModule(name)
  , m_trigger_decision_source(nullptr)
  , m_trigger_complete_sink(nullptr)
{
  register_command("conf", &FakeDataFlow::do_configure);
  register_command("start", &FakeDataFlow::do_start);
  register_command("stop", &FakeDataFlow::do_stop);
  register_command("scrap", &FakeDataFlow::do_scrap);
}

void
FakeDataFlow::init(const nlohmann::json& obj)
{
  auto ini = obj.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "trigger_decision_source") {
      m_trigger_decision_source.reset(new appfwk::DAQSource<dfmessages::TriggerDecision>(qi.inst));
    }
    if (qi.name == "trigger_complete_sink") {
      m_trigger_complete_sink.reset(new appfwk::DAQSink<dfmessages::TriggerDecisionToken>(qi.inst));
    }
  }
}

void
FakeDataFlow::get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/)
{}

void
FakeDataFlow::do_configure(const nlohmann::json& /*obj*/)
{
  m_configured_flag.store(true);
}

void
FakeDataFlow::do_start(const nlohmann::json& /*obj*/)
{
  m_running_flag.store(true);
  m_fake_data_flow_thread = std::thread(&FakeDataFlow::respond_to_trigger_decisions, this);
  pthread_setname_np(m_fake_data_flow_thread.native_handle(), "fake-data-flow");
}

void
FakeDataFlow::do_stop(const nlohmann::json& /*obj*/)
{
  m_running_flag.store(false);
  m_fake_data_flow_thread.join();
}

void
FakeDataFlow::do_scrap(const nlohmann::json& /*obj*/)
{
  m_configured_flag.store(false);
}

void
FakeDataFlow::respond_to_trigger_decisions()
{
  while (m_running_flag.load()) {
    dfmessages::TriggerDecision td;
    try {
      m_trigger_decision_source->pop(td, std::chrono::milliseconds(1000));
    } catch (appfwk::QueueTimeoutExpired&) {
      continue;
    }

    TLOG_DEBUG(1) << "Responding to decision with"
                  << " triggernumber " << td.trigger_number << " timestamp " << td.trigger_timestamp << " type "
                  << td.trigger_type << " number of links " << td.components.size();

    dfmessages::TriggerDecisionToken td_token;
    td_token.run_number = td.run_number;
    td_token.trigger_number = td.trigger_number;
    try {
      m_trigger_complete_sink->push(td_token, std::chrono::milliseconds(10));
    } catch (appfwk::QueueTimeoutExpired& e) {
      ers::error(e);
    }
  }
}

} // namespace trigger
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::FakeDataFlow)
