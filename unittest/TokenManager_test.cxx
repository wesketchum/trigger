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

/**
 * @brief Name of this test module
 */
#define BOOST_TEST_MODULE TokenManager_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <chrono>
#include <map>
#include <memory>
#include <string>

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
    std::map<std::string, appfwk::QueueConfig> queue_map = {
      { "dummy", { appfwk::QueueConfig::queue_kind::kFollyMPMCQueue, 100 } }
    };

    appfwk::QueueRegistry::get().configure(queue_map);
  }
};

BOOST_TEST_GLOBAL_FIXTURE(DAQSinkDAQSourceTestFixture);

BOOST_AUTO_TEST_CASE(Basics)
{
  using namespace std::chrono_literals;

  using sink_t = appfwk::DAQSink<dfmessages::TriggerDecisionToken>;
  using source_t = appfwk::DAQSource<dfmessages::TriggerDecisionToken>;
  auto sink = std::make_unique<sink_t>("dummy");
  auto source = std::make_unique<source_t>("dummy");

  auto livetime_counter=std::make_shared<trigger::LivetimeCounter>(trigger::LivetimeCounter::State::kPaused);
  
  int initial_tokens = 10;
  dataformats::run_number_t run_number = 1;
  trigger::TokenManager tm(source, initial_tokens, run_number, livetime_counter);

  BOOST_CHECK_EQUAL(tm.get_n_tokens(), initial_tokens);
  BOOST_CHECK_EQUAL(tm.triggers_allowed(), true);

  for (int i = 0; i < initial_tokens - 1; ++i) {
    tm.trigger_sent(i);
    BOOST_CHECK_EQUAL(tm.get_n_tokens(), initial_tokens - i - 1);
    BOOST_CHECK_EQUAL(tm.triggers_allowed(), true);
  }

  tm.trigger_sent(initial_tokens);
  BOOST_CHECK_EQUAL(tm.get_n_tokens(), 0);
  BOOST_CHECK_EQUAL(tm.triggers_allowed(), false);

  // Send a token and check that triggers become allowed again
  dfmessages::TriggerDecisionToken token;
  token.run_number = run_number;
  token.trigger_number = 1;
  sink->push(token);

  // Give TokenManager a little time to pop the token off the queue
  std::this_thread::sleep_for(100ms);
  BOOST_CHECK_EQUAL(tm.get_n_tokens(), 1);
  BOOST_CHECK_EQUAL(tm.triggers_allowed(), true);
}

BOOST_AUTO_TEST_SUITE_END()
