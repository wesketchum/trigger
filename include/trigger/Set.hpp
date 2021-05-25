/**
 * @file Set.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_INCLUDE_TRIGGER_SET_HPP_
#define TRIGGER_INCLUDE_TRIGGER_SET_HPP_

#include "dataformats/Types.hpp"
#include "triggeralgs/TriggerActivity.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"
#include "trigger/serialize.hpp"
#include "serialization/Serialization.hpp"

#include <cstdint>
#include <vector>

namespace dunedaq::trigger {

/**
 * @brief A set of TPs or TAs in a given time window, defined by its start and end times
 */
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

MSGPACK_ADD_ENUM(dunedaq::trigger::TPSet::Type)
MSGPACK_ADD_ENUM(dunedaq::trigger::TASet::Type)

DUNE_DAQ_SERIALIZE_NON_INTRUSIVE(dunedaq::trigger, TPSet, seqno, type, from_detids, start_time, end_time, objects)
DUNE_DAQ_SERIALIZE_NON_INTRUSIVE(dunedaq::trigger, TASet, seqno, type, from_detids, start_time, end_time, objects)

#endif // TRIGGER_INCLUDE_TRIGGER_SET_HPP_
