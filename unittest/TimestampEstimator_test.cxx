/**
 * @file TimestampEstimatorSystem_test.cxx  TimestampEstimatorSystem class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "trigger/TimestampEstimator.hpp"

#include <chrono>

/**
 * @brief Name of this test module
 */
#define BOOST_TEST_MODULE TimestampEstimator_test // NOLINT

#include "boost/test/unit_test.hpp"

using namespace dunedaq;

BOOST_AUTO_TEST_SUITE(BOOST_TEST_MODULE)

/**
 * @brief Initializes the QueueRegistry
 */
struct DAQSinkDAQSourceTestFixture
{
  DAQSinkDAQSourceTestFixture() {}

  void setup()
  {
    std::map<std::string, appfwk::QueueConfig> queue_map = { { "dummy", { appfwk::QueueConfig::queue_kind::kFollyMPMCQueue, 100 } } };

    appfwk::QueueRegistry::get().configure(queue_map);
  }
};

BOOST_TEST_GLOBAL_FIXTURE(DAQSinkDAQSourceTestFixture);

BOOST_AUTO_TEST_CASE(Basics)
{
  using namespace std::chrono_literals;
  
  using sink_t=appfwk::DAQSink<dfmessages::TimeSync>;
  using source_t=appfwk::DAQSource<dfmessages::TimeSync>;
  auto sink=std::make_unique<sink_t>("dummy");
  auto source=std::make_unique<source_t>("dummy");

  const uint64_t clock_frequency_hz=62'500'000;
  
  trigger::TimestampEstimator te(source, clock_frequency_hz);

  // There's no valid timestamp yet, because no TimeSync messages have
  // been received. We should immediately return with kInterrupted
  std::atomic<bool> do_not_continue_flag{false};
  BOOST_CHECK_EQUAL(te.wait_for_valid_timestamp(do_not_continue_flag), trigger::TimestampEstimatorBase::kInterrupted);

  std::atomic<bool> continue_flag{true};
  dfmessages::timestamp_t initial_ts=1;
  sink->push(dfmessages::TimeSync(initial_ts), 10ms);
  BOOST_CHECK_EQUAL(te.wait_for_valid_timestamp(continue_flag), trigger::TimestampEstimatorBase::kFinished);
  
  dfmessages::timestamp_t ts_now=te.get_timestamp_estimate();

  // Check that the timestamp has advanced a bit from the initial TimeSync, but not by "too much".
  auto too_much=clock_frequency_hz/10; // 100 ms
  BOOST_CHECK_GE(ts_now, initial_ts);
  BOOST_CHECK_LT(ts_now, initial_ts+too_much);
  
  BOOST_CHECK_EQUAL(te.wait_for_timestamp(ts_now+clock_frequency_hz, continue_flag), trigger::TimestampEstimatorBase::kFinished);

  ts_now=te.get_timestamp_estimate();
  BOOST_CHECK_EQUAL(te.wait_for_timestamp(ts_now+clock_frequency_hz, do_not_continue_flag), trigger::TimestampEstimatorBase::kInterrupted);

  // Check that the timestamp doesn't go backwards
  dfmessages::timestamp_t ts1=te.get_timestamp_estimate();
  BOOST_CHECK_GE(ts1, initial_ts);
  dfmessages::timestamp_t ts2=te.get_timestamp_estimate();
  BOOST_CHECK_GE(ts2, ts1);
  // Check that the timestamp advances
  std::this_thread::sleep_for(10ms);
  dfmessages::timestamp_t ts3=te.get_timestamp_estimate();
  BOOST_CHECK_GT(ts3, ts2);
}


BOOST_AUTO_TEST_SUITE_END()
