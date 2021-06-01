/**
 * @file TriggerActivity_serialization.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_INCLUDE_TRIGGER_TRIGGERACTIVITY_SERIALIZATION_HPP_
#define TRIGGER_INCLUDE_TRIGGER_TRIGGERACTIVITY_SERIALIZATION_HPP_

#include "triggeralgs/TriggerActivity.hpp"
#include "serialization/Serialization.hpp"
#include "trigger/TriggerPrimitive_serialization.hpp"

MSGPACK_ADD_ENUM(triggeralgs::TriggerActivityType)

MSGPACK_ADD_ENUM(triggeralgs::activity_alg_t)

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

#endif // TRIGGER_INCLUDE_TRIGGER_TRIGGERACTIVITY_SERIALIZATION_HPP_
