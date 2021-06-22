/**
 * @file TPSet.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_INCLUDE_TRIGGER_TPSET_HPP_
#define TRIGGER_INCLUDE_TRIGGER_TPSET_HPP_

#include "trigger/Set.hpp"
#include "trigger/TriggerPrimitive_serialization.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"
#include "dfmessages/GeoID_serialization.hpp"
#include "serialization/Serialization.hpp"

namespace dunedaq::trigger {

using TPSet = Set<triggeralgs::TriggerPrimitive>;

} // namespace dunedaq::trigger

MSGPACK_ADD_ENUM(dunedaq::trigger::TPSet::Type)
DUNE_DAQ_SERIALIZE_NON_INTRUSIVE(dunedaq::trigger, TPSet, seqno, type, start_time, end_time, objects)

#endif // TRIGGER_INCLUDE_TRIGGER_TPSET_HPP_
