/**
 * @file FakeDataFlow.hpp
 *
 * FakeDataFlow is a DAQModule that emulates the data flow application for the
 * sole purpose of testing trigger modules standalone.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_PLUGINS_FAKEDATAFLOW_HPP_
#define TRIGGER_PLUGINS_FAKEDATAFLOW_HPP_

#include "dfmessages/TriggerDecision.hpp"
#include "dfmessages/TriggerDecisionToken.hpp"
#include "dfmessages/Types.hpp"

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
 * @brief FakeDataFlow is a simple emulation of the data flow application
 */
class FakeDataFlow : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief FakeDataFlow Constructor
   * @param name Instance name for this FakeDataFlow instance
   */
  explicit FakeDataFlow(const std::string& name);

  FakeDataFlow(const FakeDataFlow&) = delete;            ///< FakeDataFlow is not copy-constructible
  FakeDataFlow& operator=(const FakeDataFlow&) = delete; ///< FakeDataFlow is not copy-assignable
  FakeDataFlow(FakeDataFlow&&) = delete;                 ///< FakeDataFlow is not move-constructible
  FakeDataFlow& operator=(FakeDataFlow&&) = delete;      ///< FakeDataFlow is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_pause(const nlohmann::json& obj);
  void do_resume(const nlohmann::json& obj);
  void do_scrap(const nlohmann::json& obj);

  size_t m_hold_max_size;
  size_t m_hold_min_size;
  int m_hold_min_ms;
  double m_release_randomly_prob;
  double m_forget_decision_prob;
  double m_hold_decision_prob;
  
  void respond_with_token(const dfmessages::TriggerDecision &td);
  void respond_to_trigger_decisions();
  std::thread m_fake_data_flow_thread;

  // Queue sources and sinks
  std::unique_ptr<appfwk::DAQSource<dfmessages::TriggerDecision>> m_trigger_decision_source;
  std::unique_ptr<appfwk::DAQSink<dfmessages::TriggerDecisionToken>> m_trigger_complete_sink;

  // Are we in the RUNNING state?
  std::atomic<bool> m_running_flag{ false };
  // Are we in a configured state, ie after conf and before scrap?
  std::atomic<bool> m_configured_flag{ false };
};
} // namespace trigger
} // namespace dunedaq

#endif // TRIGGER_PLUGINS_FAKEDATAFLOW_HPP_
