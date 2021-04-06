#pragma once
#include <cstdint>

namespace dunedaq {
namespace trigger {
struct TriggerPrimitive
{
  int64_t time_start = { 0 };
  int64_t time_peak = { 0 };
  int32_t time_over_threshold = { 0 };
  uint16_t channel = { 0 };
  uint32_t adc_integral = { 0 };
  uint16_t adc_peak = { 0 };
  uint16_t detid = { 0 };
  uint16_t type = { 0 };
  uint16_t algorithm = { 0 };
  uint16_t version = { 0 };
  uint32_t flag = { 0 };
};
} // namespace trigger
} // namespace dunedaq
