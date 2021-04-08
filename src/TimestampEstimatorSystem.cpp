#include "trigger/TimestampEstimatorSystem.hpp"

#include <chrono>

namespace dunedaq::trigger
{
  TimestampEstimatorSystem::TimestampEstimatorSystem(uint64_t clock_frequency_hz)
    : m_clock_frequency_hz(clock_frequency_hz)
  {
  }

  dfmessages::timestamp_t
  TimestampEstimatorSystem::get_timestamp_estimate() const
  {
    auto now=std::chrono::system_clock::now().time_since_epoch();
    auto now_us=std::chrono::duration_cast<std::chrono::microseconds>(now);
    return (m_clock_frequency_hz/1'000'000)*now_us.count();
  }
  

}

  
