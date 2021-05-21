/**
 * @file ModuleLevelTrigger.cpp ModuleLevelTrigger class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ModuleLevelTrigger.hpp"

#include "dataformats/ComponentRequest.hpp"

#include "dfmessages/TimeSync.hpp"
#include "dfmessages/TriggerDecision.hpp"
#include "dfmessages/TriggerInhibit.hpp"
#include "dfmessages/Types.hpp"
#include "logging/Logging.hpp"

#include "trigger/Issues.hpp"
#include "trigger/moduleleveltrigger/Nljs.hpp"

#include "timinglibs/TimestampEstimator.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/app/Nljs.hpp"

#include <algorithm>
#include <cassert>
#include <pthread.h>
#include <random>
#include <string>
#include <vector>

namespace dunedaq {
namespace trigger {

ModuleLevelTrigger::ModuleLevelTrigger(const std::string& name)
  : DAQModule(name)
  , m_token_source(nullptr)
  , m_trigger_decision_sink(nullptr)
  , m_last_trigger_number(0)
  , m_run_number(0)
{
  // clang-format off
  register_command("conf",   &ModuleLevelTrigger::do_configure);
  register_command("start",  &ModuleLevelTrigger::do_start);
  register_command("stop",   &ModuleLevelTrigger::do_stop);
  register_command("pause",  &ModuleLevelTrigger::do_pause);
  register_command("resume", &ModuleLevelTrigger::do_resume);
  register_command("scrap",  &ModuleLevelTrigger::do_scrap);
  // clang-format on
}

void
ModuleLevelTrigger::init(const nlohmann::json& iniobj)
{
  m_trigger_decision_sink.reset(
    new appfwk::DAQSink<dfmessages::TriggerDecision>(appfwk::queue_inst(iniobj, "trigger_decision_sink")));
  m_token_source.reset(
    new appfwk::DAQSource<dfmessages::TriggerDecisionToken>(appfwk::queue_inst(iniobj, "token_source")));
  m_candidate_source.reset(
    new appfwk::DAQSource<triggeralgs::TriggerCandidate>(appfwk::queue_inst(iniobj, "trigger_candidate_source")));
}

void
ModuleLevelTrigger::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  moduleleveltriggerinfo::Info i;

  i.tc_received_count = m_tc_received_count.load();
  i.td_sent_count = m_td_sent_count.load();
  i.td_queue_timeout_expired_err_count = m_td_queue_timeout_expired_err_count.load();
  i.td_inhibited_count = m_td_inhibited_count.load();
  i.td_paused_count = m_td_paused_count.load();
  i.td_total_count = m_td_total_count.load();

  ci.add(i);
}

void
ModuleLevelTrigger::do_configure(const nlohmann::json& confobj)
{
  auto params = confobj.get<moduleleveltrigger::ConfParams>();

  m_initial_tokens = params.initial_token_count;

  m_links.clear();
  for (auto const& link : params.links) {
    // For the future: Set APA properly
    m_links.push_back(
      dfmessages::GeoID{ dataformats::GeoID::SystemType::kTPC, 0, static_cast<uint32_t>(link) }); // NOLINT
  }

  m_configured_flag.store(true);
}

void
ModuleLevelTrigger::do_start(const nlohmann::json& startobj)
{
  m_run_number = startobj.value<dunedaq::dataformats::run_number_t>("run", 0);

  m_paused.store(true);
  m_running_flag.store(true);

  m_token_manager.reset(new TokenManager(m_token_source, m_initial_tokens, m_run_number));

  m_send_trigger_decisions_thread = std::thread(&ModuleLevelTrigger::send_trigger_decisions, this);
  pthread_setname_np(m_send_trigger_decisions_thread.native_handle(), "mlt-trig-dec");
}

void
ModuleLevelTrigger::do_stop(const nlohmann::json& /*stopobj*/)
{
  m_running_flag.store(false);
  m_send_trigger_decisions_thread.join();
  m_token_manager.reset(nullptr); // Calls TokenManager dtor
}

void
ModuleLevelTrigger::do_pause(const nlohmann::json& /*pauseobj*/)
{
  m_paused.store(true);
  TLOG() << "******* Triggers PAUSED! *********";
}

void
ModuleLevelTrigger::do_resume(const nlohmann::json& /*resumeobj*/)
{
  TLOG() << "******* Triggers RESUMED! *********";
  m_paused.store(false);
}

void
ModuleLevelTrigger::do_scrap(const nlohmann::json& /*stopobj*/)
{
  m_configured_flag.store(false);
}

dfmessages::TriggerDecision
ModuleLevelTrigger::create_decision(const triggeralgs::TriggerCandidate& tc)
{
  dfmessages::TriggerDecision decision;
  decision.trigger_number = m_last_trigger_number + 1;
  decision.run_number = m_run_number;
  decision.trigger_timestamp = tc.time_candidate;
  // TODO: work out what to set this to
  decision.trigger_type = 1; // m_trigger_type;
  decision.readout_type = dfmessages::ReadoutType::kLocalized;

  for (auto link : m_links) {
    dfmessages::ComponentRequest request;
    request.component = link;
    // TODO: set these from some config map
    request.window_begin = tc.time_candidate;
    request.window_end = tc.time_candidate + 1000;

    decision.components.push_back(request);
  }

  return decision;
}

void
ModuleLevelTrigger::send_trigger_decisions()
{

  // We get here at start of run, so reset the trigger number
  m_last_trigger_number = 0;

  // OpMon.
  m_tc_received_count.store(0);
  m_td_sent_count.store(0);
  m_td_queue_timeout_expired_err_count.store(0);
  m_td_inhibited_count.store(0);
  m_td_paused_count.store(0);
  m_td_total_count.store(0);

  while (true) {
    triggeralgs::TriggerCandidate tc;
    try {
      m_candidate_source->pop(tc, std::chrono::milliseconds(100));
      ++m_tc_received_count;
    } catch (appfwk::QueueTimeoutExpired&) {
      // The condition to exit the loop is that we've been stopped and
      // there's nothing left on the input queue
      if (!m_running_flag.load()) {
        break;
      } else {
        continue;
      }
    }

    bool tokens_allow_triggers = m_token_manager->triggers_allowed();

    if (!m_paused.load() && tokens_allow_triggers) {

      dfmessages::TriggerDecision decision = create_decision(tc);

      TLOG_DEBUG(1) << "Pushing a decision with triggernumber " << decision.trigger_number << " timestamp "
                    << decision.trigger_timestamp << " number of links " << decision.components.size();

      // Have to notify the token manager of the trigger _before_
      // actually pushing it, otherwise the DF could reply with
      // TriggerComplete before we get to the notification. If that
      // happens, then the token manager sees a TriggerComplete for a
      // token it doesn't know about, and ignores it
      m_token_manager->trigger_sent(decision.trigger_number);
      try {
        m_trigger_decision_sink->push(decision, std::chrono::milliseconds(10));
        m_td_sent_count++;
      } catch (appfwk::QueueTimeoutExpired& e) {
        m_td_queue_timeout_expired_err_count++;
        ers::error(e);
      }

      decision.trigger_number++;
      m_last_trigger_number++;
    } else if (!tokens_allow_triggers) {
      TLOG_DEBUG(1) << "There are no Tokens available. Not sending a TriggerDecision for candidate timestamp "
                    << tc.time_candidate;
      m_td_inhibited_count++;
    } else {
      ++m_td_paused_count;
      TLOG_DEBUG(1) << "Triggers are paused. Not sending a TriggerDecision ";
    }
    m_td_total_count++;
  }

  TLOG() << "Received " << m_tc_received_count << " TCs. Sent " << m_td_sent_count.load() << " TDs. "
         << m_td_paused_count << " TDs were created during pause, and " << m_td_inhibited_count.load()
         << " TDs were inhibited.";
}

} // namespace trigger
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::ModuleLevelTrigger)

// Local Variables:
// c-basic-offset: 2
// End:
