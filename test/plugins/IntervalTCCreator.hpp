/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_TEST_PLUGINS_INTERVALTCCREATOR_HPP_
#define TRIGGER_TEST_PLUGINS_INTERVALTCCREATOR_HPP_

#include "trigger/TokenManager.hpp"

#include "trigger/intervaltccreator/Nljs.hpp"

#include "triggeralgs/TriggerCandidate.hpp"

#include "daqdataformats/GeoID.hpp"
#include "dfmessages/TimeSync.hpp"
#include "dfmessages/TriggerDecision.hpp"
#include "dfmessages/TriggerDecisionToken.hpp"
#include "dfmessages/TriggerInhibit.hpp"
#include "dfmessages/Types.hpp"

#include "timinglibs/TimestampEstimator.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace dunedaq {

namespace trigger {

/**
 * @brief IntervalTCCreator creates TriggerCandidates at regular
 * intervals, based on input from a TimeSync queue or the system
 * clock. The TCs can be fed directly into the MLT, to allow testing
 * the MLT separately from the Timing Message -> TC conversion logic
 */
class IntervalTCCreator : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief IntervalTCCreator Constructor
   * @param name Instance name for this IntervalTCCreator instance
   */
  explicit IntervalTCCreator(const std::string& name);

  IntervalTCCreator(const IntervalTCCreator&) = delete;            ///< IntervalTCCreator is not copy-constructible
  IntervalTCCreator& operator=(const IntervalTCCreator&) = delete; ///< IntervalTCCreator is not copy-assignable
  IntervalTCCreator(IntervalTCCreator&&) = delete;                 ///< IntervalTCCreator is not move-constructible
  IntervalTCCreator& operator=(IntervalTCCreator&&) = delete;      ///< IntervalTCCreator is not move-assignable

  void init(const nlohmann::json& iniobj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_scrap(const nlohmann::json& obj);

  void send_trigger_candidates();
  std::thread m_send_trigger_candidates_thread;

  std::unique_ptr<timinglibs::TimestampEstimatorBase> m_timestamp_estimator;

  // Create the next trigger decision
  triggeralgs::TriggerCandidate create_candidate(dfmessages::timestamp_t timestamp);

  // Queue sources and sinks
  std::unique_ptr<appfwk::DAQSource<dfmessages::TimeSync>> m_time_sync_source;
  std::unique_ptr<appfwk::DAQSink<triggeralgs::TriggerCandidate>> m_trigger_candidate_sink;

  intervaltccreator::ConfParams m_conf;

  dfmessages::trigger_number_t m_last_trigger_number;
  dfmessages::run_number_t m_run_number;

  // Are we in the RUNNING state?
  std::atomic<bool> m_running_flag{ false };
  // Are we in a configured state, ie after conf and before scrap?
  std::atomic<bool> m_configured_flag{ false };
};
} // namespace trigger
} // namespace dunedaq

#endif // TRIGGER_TEST_PLUGINS_INTERVALTCCREATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
