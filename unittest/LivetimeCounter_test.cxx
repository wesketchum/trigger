/**
 * @file TokenManager_test.cxx  TokenManager class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "trigger/LivetimeCounter.hpp"
#include "trigger/TokenManager.hpp"

#include <chrono>

/**
 * @brief Name of this test module
 */
#define BOOST_TEST_MODULE LivetimeCounter_test // NOLINT

#include "boost/test/unit_test.hpp"

using namespace dunedaq;

BOOST_AUTO_TEST_SUITE(BOOST_TEST_MODULE)

BOOST_AUTO_TEST_CASE(Basics)
{
  using namespace std::chrono_literals;
  using namespace dunedaq::trigger;
  
  LivetimeCounter lc(LivetimeCounter::State::kPaused);
  std::this_thread::sleep_for(100ms);
  auto m=lc.get_time_map();
  BOOST_CHECK_EQUAL(m.size(), 3);
  // Check that some time has accumulated for the "paused" state only
  BOOST_CHECK_EQUAL(m.at(LivetimeCounter::State::kLive), 0);
  BOOST_CHECK_EQUAL(m.at(LivetimeCounter::State::kDead), 0);
  BOOST_CHECK_GT(m.at(LivetimeCounter::State::kPaused), 99);

  lc.set_state(LivetimeCounter::State::kLive);
  std::this_thread::sleep_for(200ms);
  // Allow 5ms each way?
  BOOST_CHECK_GT(lc.get_time(LivetimeCounter::State::kLive), 195);
  BOOST_CHECK_LT(lc.get_time(LivetimeCounter::State::kLive), 205);
  
  lc.set_state(LivetimeCounter::State::kDead);
  std::this_thread::sleep_for(100ms);
  BOOST_CHECK_GT(lc.get_time(LivetimeCounter::State::kDead),  95);
  BOOST_CHECK_LT(lc.get_time(LivetimeCounter::State::kDead), 105);

  // Go back to the paused state and check that we continue to
  // accumulate (rather than resetting)
  lc.set_state(LivetimeCounter::State::kPaused);
  std::this_thread::sleep_for(200ms);
  BOOST_CHECK_GT(lc.get_time(LivetimeCounter::State::kPaused), 295);
  BOOST_CHECK_LT(lc.get_time(LivetimeCounter::State::kPaused), 305);
  
}


BOOST_AUTO_TEST_SUITE_END()
