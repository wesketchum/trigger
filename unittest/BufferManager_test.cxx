/**
 * @file BufferManager_test.cxx  BufferManager class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "trigger/BufferManager.hpp"

#include "dfmessages/DataRequest.hpp"

#include <random>
#include <chrono>
#include <thread>

/**
 * @brief Name of this test module
 */

#define BOOST_TEST_MODULE BufferManager_test // NOLINT

#include "boost/test/unit_test.hpp"

using namespace dunedaq;

BOOST_AUTO_TEST_SUITE(BOOST_TEST_MODULE)

BOOST_AUTO_TEST_CASE(Basics)
{
  long unsigned int buffer_size = 100;
  trigger::BufferManager bm(buffer_size);
  trigger::TPSet input_tpset;

  // Generate a random TPSet
  std::default_random_engine generator;
  std::uniform_int_distribution<int> uniform(0, 100);

  input_tpset.start_time = 1000 + uniform(generator);
  input_tpset.end_time   = input_tpset.start_time + uniform(generator);

  // Store a TPSet
  BOOST_CHECK_EQUAL(bm.add(input_tpset), true);

  // Generate a random data request 
  trigger::BufferManager::data_request_output requested_tpset;
  dfmessages::DataRequest input_data_request;

  // Request data
  requested_tpset = bm.get_tpsets_in_window( input_data_request.window_begin, input_data_request.window_end );
  BOOST_CHECK_EQUAL(requested_tpset.ds_outcome, trigger::BufferManager::kSuccess);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

BOOST_AUTO_TEST_SUITE_END()
