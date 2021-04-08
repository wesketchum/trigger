#include "trigger/TimestampEstimatorBase.hpp"

#include <thread>

namespace dunedaq::trigger
{
TimestampEstimatorBase::WaitStatus
TimestampEstimatorBase::wait_for_valid_timestamp(std::atomic<bool>& continue_flag)
{
  if (!continue_flag.load())
    return TimestampEstimatorBase::kInterrupted;

  while (continue_flag.load() && get_timestamp_estimate() == dfmessages::TypeDefaults::s_invalid_timestamp) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (!continue_flag.load())
      return TimestampEstimatorBase::kInterrupted;
  }

  return TimestampEstimatorBase::kFinished;
}

TimestampEstimatorBase::WaitStatus
TimestampEstimatorBase::wait_for_timestamp(dfmessages::timestamp_t ts, std::atomic<bool>& continue_flag)
{
  if (!continue_flag.load())
    return TimestampEstimatorBase::kInterrupted;

  while (continue_flag.load() &&
         (get_timestamp_estimate() < ts || get_timestamp_estimate() == dfmessages::TypeDefaults::s_invalid_timestamp)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (!continue_flag.load())
      return TimestampEstimatorBase::kInterrupted;
  }

  return TimestampEstimatorBase::kFinished;
}

}
