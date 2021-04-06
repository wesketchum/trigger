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
#include "trigger/TimestampEstimator.hpp"
#include "trigger/moduleleveltrigger/Nljs.hpp"
// #include "trigger/moduleleveltriggerinfo/Nljs.hpp"

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
  , m_time_sync_source(nullptr)
  , m_token_source(nullptr)
  , m_trigger_decision_sink(nullptr)
  , m_last_trigger_number(0)
  , m_run_number(0)
{
  register_command("conf", &ModuleLevelTrigger::do_configure);
  register_command("start", &ModuleLevelTrigger::do_start);
  register_command("stop", &ModuleLevelTrigger::do_stop);
  register_command("pause", &ModuleLevelTrigger::do_pause);
  register_command("resume", &ModuleLevelTrigger::do_resume);
  register_command("scrap", &ModuleLevelTrigger::do_scrap);
}

void
ModuleLevelTrigger::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "time_sync_source") {
      m_time_sync_source.reset(new appfwk::DAQSource<dfmessages::TimeSync>(qi.inst));
    }
    if (qi.name == "trigger_decision_sink") {
      m_trigger_decision_sink.reset(new appfwk::DAQSink<dfmessages::TriggerDecision>(qi.inst));
    }
    if (qi.name == "token_source") {
      m_token_source.reset(new appfwk::DAQSource<dfmessages::TriggerDecisionToken>(qi.inst));
    }
  }
}

void
ModuleLevelTrigger::get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/)
{
  // moduleleveltriggerinfo::Info tde;

  // tde.triggers = m_trigger_count_tot.load();
  // tde.new_triggers = m_trigger_count.exchange(0);
  // tde.inhibited = m_inhibited_trigger_count_tot.load();
  // tde.new_inhibited = m_inhibited_trigger_count.exchange(0);

  // ci.add(tde);
}

void
ModuleLevelTrigger::do_configure(const nlohmann::json& confobj)
{
  auto params = confobj.get<moduleleveltrigger::ConfParams>();

  m_min_readout_window_ticks = params.min_readout_window_ticks;
  m_max_readout_window_ticks = params.max_readout_window_ticks;
  m_trigger_window_offset = params.trigger_window_offset;

  m_min_links_in_request = params.min_links_in_request;
  m_max_links_in_request = params.max_links_in_request;

  m_trigger_interval_ticks.store(params.trigger_interval_ticks);

  m_trigger_offset = params.trigger_offset;
  trigger_delay_ticks_ = params.trigger_delay_ticks;
  m_clock_frequency_hz = params.clock_frequency_hz;
  m_repeat_trigger_count = params.repeat_trigger_count;

  m_stop_burst_count = params.stop_burst_count;
  m_initial_tokens = params.initial_token_count;

  m_links.clear();
  for (auto const& link : params.links) {
    // For the future: Set APA properly
    m_links.push_back(dfmessages::GeoID{ 0, static_cast<uint32_t>(link) }); // NOLINT
  }

  // Sanity-check the values
  if (m_min_readout_window_ticks > m_max_readout_window_ticks || m_min_links_in_request > m_max_links_in_request) {
    throw InvalidConfiguration(ERS_HERE);
  }

  m_configured_flag.store(true);
}

void
ModuleLevelTrigger::do_start(const nlohmann::json& startobj)
{
  m_run_number = startobj.value<dunedaq::dataformats::run_number_t>("run", 0);

  m_paused.store(true);
  m_running_flag.store(true);

  m_timestamp_estimator.reset(new TimestampEstimator(m_time_sync_source, m_clock_frequency_hz));
  m_token_manager.reset(new TokenManager(m_token_source, m_initial_tokens, m_run_number));

  m_send_trigger_decisions_thread = std::thread(&ModuleLevelTrigger::send_trigger_decisions, this);
  pthread_setname_np(m_send_trigger_decisions_thread.native_handle(), "tde-trig-dec");
}

void
ModuleLevelTrigger::do_stop(const nlohmann::json& /*stopobj*/)
{
  m_running_flag.store(false);

  m_send_trigger_decisions_thread.join();

  m_timestamp_estimator.reset(nullptr); // Calls TimestampEstimator dtor
  m_token_manager.reset(nullptr);       // Calls TokenManager dtor
}

void
ModuleLevelTrigger::do_pause(const nlohmann::json& /*pauseobj*/)
{
  m_paused.store(true);
  TLOG() << "******* Triggers PAUSED! *********";
}

void
ModuleLevelTrigger::do_resume(const nlohmann::json& resumeobj)
{
  auto params = resumeobj.get<moduleleveltrigger::ResumeParams>();
  m_trigger_interval_ticks.store(params.trigger_interval_ticks);

  TLOG() << "******* Triggers RESUMED! *********";
  m_paused.store(false);
}

void
ModuleLevelTrigger::do_scrap(const nlohmann::json& /*stopobj*/)
{
  m_configured_flag.store(false);
}

dfmessages::TriggerDecision
ModuleLevelTrigger::create_decision(dfmessages::timestamp_t timestamp)
{
  static std::default_random_engine random_engine(m_run_number);
  static std::uniform_int_distribution<int> n_links_dist(m_min_links_in_request,
                                                         std::min((size_t)m_max_links_in_request, m_links.size()));
  static std::uniform_int_distribution<dfmessages::timestamp_t> window_ticks_dist(m_min_readout_window_ticks,
                                                                                  m_max_readout_window_ticks);

  dfmessages::TriggerDecision decision;
  decision.trigger_number = m_last_trigger_number + 1;
  decision.run_number = m_run_number;
  decision.trigger_timestamp = timestamp;
  decision.trigger_type = m_trigger_type;

  int n_links = n_links_dist(random_engine);

  std::vector<dfmessages::GeoID> this_links;
  std::sample(m_links.begin(), m_links.end(), std::back_inserter(this_links), n_links, random_engine);

  for (auto link : this_links) {
    dfmessages::ComponentRequest request;
    request.component = link;
    request.window_begin = timestamp - m_trigger_window_offset;
    request.window_end = request.window_begin + window_ticks_dist(random_engine);

    decision.components.push_back(request);
  }

  return decision;
}

void
ModuleLevelTrigger::send_trigger_decisions()
{

  // We get here at start of run, so reset the trigger number
  m_last_trigger_number = 0;
  m_trigger_count.store(0);
  m_trigger_count_tot.store(0);
  m_inhibited_trigger_count.store(0);
  m_inhibited_trigger_count_tot.store(0);

  // Wait for there to be a valid timestamp estimate before we start
  if (m_timestamp_estimator->wait_for_valid_timestamp(m_running_flag) == TimestampEstimator::kInterrupted) {
    return;
  }

  dfmessages::timestamp_t ts = m_timestamp_estimator->get_timestamp_estimate();

  TLOG_DEBUG(1) << "Delaying trigger decision sending by " << trigger_delay_ticks_ << " ticks";
  // Round up to the next multiple of trigger_interval_ticks_
  dfmessages::timestamp_t next_trigger_timestamp =
    (ts / m_trigger_interval_ticks.load() + 1) * m_trigger_interval_ticks.load() + m_trigger_offset;
  TLOG_DEBUG(1) << "Initial timestamp estimate is " << ts << ", next_trigger_timestamp is " << next_trigger_timestamp;

  assert(next_trigger_timestamp > ts);

  while (true) {
    if (m_timestamp_estimator->wait_for_timestamp(next_trigger_timestamp + trigger_delay_ticks_, m_running_flag) ==
        TimestampEstimator::kInterrupted) {
      break;
    }

    bool tokens_allow_triggers = m_token_manager->triggers_allowed();
    if (!m_paused.load() && tokens_allow_triggers) {

      dfmessages::TriggerDecision decision = create_decision(next_trigger_timestamp);

      for (int i = 0; i < m_repeat_trigger_count; ++i) {
        TLOG_DEBUG(1) << "At timestamp " << m_timestamp_estimator->get_timestamp_estimate()
                      << ", pushing a decision with triggernumber " << decision.trigger_number << " timestamp "
                      << decision.trigger_timestamp << " number of links " << decision.components.size();
        m_trigger_decision_sink->push(decision, std::chrono::milliseconds(10));
        m_token_manager->trigger_sent(decision.trigger_number);
        decision.trigger_number++;
        m_last_trigger_number++;
        m_trigger_count++;
        m_trigger_count_tot++;
      }
    } else if (!tokens_allow_triggers) {
      TLOG_DEBUG(1) << "There are no Tokens available. Not sending a TriggerDecision for timestamp "
                    << next_trigger_timestamp;
      m_inhibited_trigger_count++;
      m_inhibited_trigger_count_tot++;
    } else {
      TLOG_DEBUG(1) << "Triggers are paused. Not sending a TriggerDecision for timestamp " << next_trigger_timestamp;
    }

    next_trigger_timestamp += m_trigger_interval_ticks.load();
  }

  // We get here after the stop command is received. We send out
  // m_stop_burst_count triggers in one go, so that there are triggers
  // in-flight in the system during the stopping transition. This is
  // intended to allow tests that all of the queues are correctly
  // drained elsewhere in the system during the stop transition
  if (m_stop_burst_count) {
    TLOG_DEBUG(0) << "Sending " << m_stop_burst_count << " triggers at stop";
    dfmessages::TriggerDecision decision = create_decision(next_trigger_timestamp);

    for (int i = 0; i < m_stop_burst_count; ++i) {
      m_trigger_decision_sink->push(decision, std::chrono::milliseconds(10));
      decision.trigger_number++;
      m_last_trigger_number++;
      m_trigger_count++;
      m_trigger_count_tot++;
    }
  }
}

} // namespace trigger
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::ModuleLevelTrigger)

// Local Variables:
// c-basic-offset: 2
// End:
