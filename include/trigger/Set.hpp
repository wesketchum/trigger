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

} // namespace dunedaq::trigger

#endif // TRIGGER_INCLUDE_TRIGGER_SET_HPP_
