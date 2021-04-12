/**
 * @file TimestampEstimatorSystem.hpp TimestampEstimatorSystem Class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_SRC_TRIGGER_TIMESTAMPESTIMATORBASE_HPP_
#define TRIGGER_SRC_TRIGGER_TIMESTAMPESTIMATORBASE_HPP_

#include "dfmessages/Types.hpp"

#include <atomic>

namespace dunedaq {
namespace trigger {

/**
 * @brief TimestampEstimatorBase is the base class for timestamp-based
 * logic in test systems where the current timestamp must be estimated
 * somehow (eg, because there is no hardware timing system).
 **/
class TimestampEstimatorBase
{
public:
  virtual dfmessages::timestamp_t get_timestamp_estimate() const = 0;

  enum WaitStatus
  {
    kFinished,
    kInterrupted
  };

  /**
     Wait for the current timestamp estimate to become valid, or for
     continue_flag to become false. The timestamp becomes valid once at
     least one TimeSync message has been received.

     Returns kFinished if the timestamp became valid, or kInterrupted if continue_flag became false first
  */
  WaitStatus wait_for_valid_timestamp(std::atomic<bool>& continue_flag);

  /**
     Wait for the current timestamp estimate to reach ts, or for
     continue_flag to become false.

     Returns kFinished if the timestamp became valid, or kInterrupted if continue_flag became false first
  */
  WaitStatus wait_for_timestamp(dfmessages::timestamp_t ts, std::atomic<bool>& continue_flag);

};
  
}
}

#endif // TRIGGER_SRC_TRIGGER_TIMESTAMPESTIMATORBASE_HPP_
