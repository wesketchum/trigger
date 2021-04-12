/**
 * @file TimestampEstimatorSystem.hpp TimestampEstimatorSystem Class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_SRC_TRIGGER_TIMESTAMPESTIMATORSYSTEM_HPP_
#define TRIGGER_SRC_TRIGGER_TIMESTAMPESTIMATORSYSTEM_HPP_

#include "trigger/TimestampEstimatorBase.hpp"


namespace dunedaq {
namespace trigger {

/**
 * @brief TimestampEstimatorSystem is an implementation of
 * TimestampEstimatorBase that uses the system clock to give the current timestamp
 **/
class TimestampEstimatorSystem : public TimestampEstimatorBase
{
public:
  
  TimestampEstimatorSystem(uint64_t clock_frequency_hz);

  dfmessages::timestamp_t get_timestamp_estimate() const override;

private:
  uint64_t m_clock_frequency_hz; // NOLINT
};

}
}

#endif // TRIGGER_SRC_TRIGGER_TIMESTAMPESTIMATORSYSTEM_HPP_
