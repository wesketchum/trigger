#ifndef TRIGGER_INCLUDE_TRIGGER_SET_HPP_
#define TRIGGER_INCLUDE_TRIGGER_SET_HPP_

#include "dataformats/Types.hpp"
#include "triggeralgs/TriggerActivity.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"
#include "triggeralgs/TriggerCandidate.hpp"
#include "serialization/Serialization.hpp"

#include <cstdint>
#include <vector>

namespace dunedaq::trigger {

template<class T>
class Set {
public:

  enum Type {
    kUnknown = 0,
    kPayload = 1,
    kHeartbeat = 2
  };

  // An incremental count of how many Sets have been produced by this source
  uint32_t seqno{0};

  // Whether this Set is a regular bag-of-objects or a heartbeat
  Type type{kUnknown};

  // The detids that were inspected to form this Set
  std::vector<uint16_t> from_detids;

  // The earliest timestamp inspected to form this Set
  dataformats::timestamp_t start_time{0};
  // The latest timestamp inspected to form this Set
  dataformats::timestamp_t end_time{0};

  // The TPs/TAs themselves. Needs a better name!
  std::vector<T> objects;
};

using TPSet = Set<triggeralgs::TriggerPrimitive>;
using TASet = Set<triggeralgs::TriggerActivity>;

} // namespace dunedaq::trigger

DUNE_DAQ_SERIALIZE_NON_INTRUSIVE(triggeralgs, TriggerPrimitive,
                                 time_start,
                                 time_peak,
                                 time_over_threshold,
                                 channel,
                                 adc_integral,
                                 adc_peak,
                                 detid,
                                 type,
                                 algorithm,
                                 version,
                                 flag)

DUNE_DAQ_SERIALIZE_NON_INTRUSIVE(triggeralgs, TriggerActivity,
                                 time_start,
                                 time_end,
                                 time_peak,
                                 time_activity,
                                 channel_start,
                                 channel_end,
                                 channel_peak,
                                 adc_integral,
                                 adc_peak,
                                 detid,
                                 type,
                                 algorithm,
                                 version,
                                 tp_list)

DUNE_DAQ_SERIALIZE_NON_INTRUSIVE(triggeralgs, TriggerCandidate,
                                 time_start,
                                 time_end,
                                 time_candidate,
                                 detid,
                                 type,
                                 algorithm,
                                 version,
                                 ta_list)

DUNE_DAQ_SERIALIZE_NON_INTRUSIVE(dunedaq::trigger, TPSet, seqno, from_detids, start_time, end_time, objects)
DUNE_DAQ_SERIALIZE_NON_INTRUSIVE(dunedaq::trigger, TASet, seqno, from_detids, start_time, end_time, objects)

#endif // TRIGGER_INCLUDE_TRIGGER_SET_HPP_
