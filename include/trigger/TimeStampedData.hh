#pragma once
#include <cstdint>

namespace dunedaq {
namespace trigger {
struct TimeStampedData
{
  uint32_t signal_type = { 0 };
  uint64_t time_stamp = { 0 };
  uint32_t counter = { 0 };
};
} // namespace trigger
} // namespace dunedaq
