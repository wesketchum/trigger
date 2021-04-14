/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_TEST_PLUGINS_FAKETIMESTAMPDATAGENERATOR_HPP_
#define TRIGGER_TEST_PLUGINS_FAKETIMESTAMPDATAGENERATOR_HPP_

#include "dune-trigger-algs/TimeStampedData.hh"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/ThreadHelper.hpp"

#include <ers/Issue.hpp>

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <random>

namespace dunedaq {
namespace trigger {

/**
 * @brief RandomDataListGenerator creates vectors of ints and writes
 * them to the configured output queues.
 */
class FakeTimeStampDataGenerator : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief FakeTimeStampDataGenerator Constructor
   * @param name Instance name for this FakeTimeStampDataGenerator instance
   */
  explicit FakeTimeStampDataGenerator(const std::string& name);

  FakeTimeStampDataGenerator(const FakeTimeStampDataGenerator&) =
    delete; ///< FakeTimeStampDataGenerator is not copy-constructible
  FakeTimeStampDataGenerator& operator=(const FakeTimeStampDataGenerator&) =
    delete; ///< FakeTimeStampDataGenerator is not copy-assignable
  FakeTimeStampDataGenerator(FakeTimeStampDataGenerator&&) =
    delete; ///< FakeTimeStampDataGenerator is not move-constructible
  FakeTimeStampDataGenerator& operator=(FakeTimeStampDataGenerator&&) =
    delete; ///< FakeTimeStampDataGenerator is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_scrap(const nlohmann::json& obj);

  // Threading
  dunedaq::appfwk::ThreadHelper thread_;
  void do_work(std::atomic<bool>&);

  // Configuration
  using sink_t = dunedaq::appfwk::DAQSink<triggeralgs::TimeStampedData>;
  std::unique_ptr<sink_t> outputQueue_;
  std::chrono::milliseconds queueTimeout_;

  // Random Generatior
  std::vector<triggeralgs::TimeStampedData> GetTimestamp();
  std::default_random_engine generator;
  std::uniform_int_distribution<int> rdm_signaltype = std::uniform_int_distribution<int>    (0, 2);
};
} // namespace trigger
  /*
ERS_DECLARE_ISSUE_BASE(trigger,
                       NoOutputQueuesAvailableWarning,
                       appfwk::GeneralDAQModuleIssue,
                       "No output queues were available, so the generated list of integers will be dropped. Has initialization been successfully completed?",
                       ((std::string)name),
                       ERS_EMPTY)
  */
} // namespace dunedaq

#endif // TRIGGER_TEST_PLUGINS_FAKETIMESTAMPDATAGENERATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
