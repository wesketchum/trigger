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

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/app/Nljs.hpp"

#include <chrono>
#include <random>
#include <string>
#include <vector>

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
  m_trigger_decision_source.reset(
    new appfwk::DAQSource<dfmessages::TriggerDecision>(appfwk::queue_inst(obj, "trigger_decision_source")));
  m_trigger_complete_sink.reset(
    new appfwk::DAQSink<dfmessages::TriggerDecisionToken>(appfwk::queue_inst(obj, "trigger_complete_sink")));
}

void
FakeDataFlow::get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/)
{}

void
FakeDataFlow::do_configure(const nlohmann::json& obj)
{
  auto params = obj.get<fakedataflow::ConfParams>();
  m_configured_flag.store(true);
  m_hold_max_size = params.hold_max_size;
  m_hold_min_size = params.hold_min_size;
  m_hold_min_ms = params.hold_min_ms;
  m_release_randomly_prob = params.release_randomly_prob;
  m_forget_decision_prob = params.forget_decision_prob;
  m_hold_decision_prob = params.hold_decision_prob;
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
FakeDataFlow::respond_with_token(const dfmessages::TriggerDecision& td)
{
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

/**

Logic goes like this:
1) TriggerDecision is received
2) A flat probability to forget the decision and go to 1
3) A flat probability to hold the decision in a queue and go to 1
4) A TriggerDecisionToken is sent

The held decision queue is filled until it reaches a predetermined size and
then empties itself with the following logic until it reaches a predetermined
minimum size.
1) A flat probability to reply to a random held decision
2) Otherwise reply to the oldest held decision

*/
void
FakeDataFlow::respond_to_trigger_decisions()
{
  std::default_random_engine generator;
  std::uniform_real_distribution<double> uniform(0.0, 1.0);
  std::vector<dfmessages::TriggerDecision> held_decisions;
  bool hold_full = false;
  std::chrono::steady_clock::time_point hold_full_time;
  while (m_running_flag.load()) {
    if (held_decisions.size() && held_decisions.size() >= m_hold_max_size) {
      if (hold_full) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        int delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - hold_full_time).count();
        if (delta > m_hold_min_ms) {
          TLOG_DEBUG(1) << "Now releasing held TDs";
          while (held_decisions.size() > m_hold_min_size) {
            int i;
            if (uniform(generator) < m_release_randomly_prob) {
              i = static_cast<int>(uniform(generator) * held_decisions.size());
            } else {
              i = 0;
            }
            respond_with_token(held_decisions[i]);
            held_decisions.erase(held_decisions.begin() + i);
          }
          hold_full = false;
        }
      } else {
        TLOG_DEBUG(1) << "Held TD queue full, waiting " << m_hold_min_ms << " ms to release";
        hold_full_time = std::chrono::steady_clock::now();
        hold_full = true;
      }
    }

    dfmessages::TriggerDecision td;
    try {
      m_trigger_decision_source->pop(td, std::chrono::milliseconds(100));
    } catch (appfwk::QueueTimeoutExpired&) {
      continue;
    }

    if (uniform(generator) < m_forget_decision_prob) {
      TLOG_DEBUG(1) << "Forgetting decision with"
                    << " triggernumber " << td.trigger_number << " timestamp " << td.trigger_timestamp << " type "
                    << td.trigger_type << " number of links " << td.components.size();
      continue;
    }

    if (uniform(generator) < m_hold_decision_prob) {
      TLOG_DEBUG(1) << "Holding decision with"
                    << " triggernumber " << td.trigger_number << " timestamp " << td.trigger_timestamp << " type "
                    << td.trigger_type << " number of links " << td.components.size();
      held_decisions.push_back(td);
      continue;
    }

    respond_with_token(td);
  }
}

} // namespace trigger
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::FakeDataFlow)
