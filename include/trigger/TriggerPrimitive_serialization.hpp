/**
 * @file TriggerPrimitive_serialization.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_INCLUDE_TRIGGER_TRIGGERPRIMITIVE_SERIALIZATION_HPP_
#define TRIGGER_INCLUDE_TRIGGER_TRIGGERPRIMITIVE_SERIALIZATION_HPP_

#include "triggeralgs/TriggerPrimitive.hpp"
#include "serialization/Serialization.hpp"
#include "triggeralgs/Types.hpp"

MSGPACK_ADD_ENUM(triggeralgs::TriggerPrimitiveType)

MSGPACK_ADD_ENUM(triggeralgs::primitive_alg_t)

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

#endif // TRIGGER_INCLUDE_TRIGGER_TRIGGERPRIMITIVE_SERIALIZATION_HPP_
