/**
 * @file TPFragment_test.cxx  Unit Tests for reading and writing code for TriggerPrimitivesFragment
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "dataformats/trigger/TriggerPrimitivesFragment.hpp"
#include "trigger/FragmentMakers.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <random>

/**
 * @brief Name of this test module
 */
#define BOOST_TEST_MODULE TPFragment_test // NOLINT

#include "boost/test/unit_test.hpp"

using namespace dunedaq;

namespace triggeralgs {
std::ostream&
boost_test_print_type(std::ostream& os, TriggerPrimitive::Algorithm const& alg)
{
  using underlying_t = std::underlying_type<TriggerPrimitive::Algorithm>::type;
  return os << static_cast<underlying_t>(alg);
}
}

BOOST_AUTO_TEST_SUITE(BOOST_TEST_MODULE)

BOOST_AUTO_TEST_CASE(Basics)
{

  std::default_random_engine generator;
  std::uniform_int_distribution<int> uniform(0, 1000);

  int n_trigger_primitives = 10;
  std::vector<triggeralgs::TriggerPrimitive> tps;
  for (int i = 0; i < n_trigger_primitives; ++i) {
    triggeralgs::TriggerPrimitive tp;
    tp.time_start = 1234963454;
    tp.time_peak = tp.time_start + 10000;
    tp.time_over_threshold = uniform(generator);
    tp.channel = uniform(generator);
    tp.adc_integral = uniform(generator);
    tp.adc_peak = uniform(generator);
    tp.detid = i;
    tp.type = triggeralgs::TriggerPrimitive::Type::kUnknown;
    tp.algorithm = triggeralgs::TriggerPrimitive::Algorithm::kTPCDefault;
    tp.version = 1;
    tp.flag = 1;
    tps.push_back(tp);
  }

  std::unique_ptr<dataformats::Fragment> frag = dunedaq::trigger::make_fragment(tps);
  dataformats::TriggerPrimitivesFragment* tpf =
    reinterpret_cast<dataformats::TriggerPrimitivesFragment*>(frag->get_data());

  BOOST_REQUIRE_EQUAL(tpf->n_trigger_primitives, n_trigger_primitives);
  BOOST_REQUIRE_EQUAL(tpf->version, 1);

  std::vector<triggeralgs::TriggerPrimitive> tps_out = trigger::read_fragment_to_trigger_primitives(frag.get());
  BOOST_REQUIRE_EQUAL(tps_out.size(), tps.size());

  for (int i = 0; i < n_trigger_primitives; ++i) {
    BOOST_CHECK_EQUAL(tps[i].time_start, tps_out[i].time_start);
    BOOST_CHECK_EQUAL(tps[i].time_peak, tps_out[i].time_peak);
    BOOST_CHECK_EQUAL(tps[i].time_over_threshold, tps_out[i].time_over_threshold);
    BOOST_CHECK_EQUAL(tps[i].channel, tps_out[i].channel);
    BOOST_CHECK_EQUAL(tps[i].adc_integral, tps_out[i].adc_integral);
    BOOST_CHECK_EQUAL(tps[i].adc_peak, tps_out[i].adc_peak);
    BOOST_CHECK_EQUAL(tps[i].detid, tps_out[i].detid);
    BOOST_CHECK_EQUAL(tps[i].type, tps_out[i].type);
    BOOST_CHECK_EQUAL(tps[i].algorithm, tps_out[i].algorithm);
    BOOST_CHECK_EQUAL(tps[i].version, tps_out[i].version);
    BOOST_CHECK_EQUAL(tps[i].flag, tps_out[i].flag);
  }
}

BOOST_AUTO_TEST_SUITE_END()
