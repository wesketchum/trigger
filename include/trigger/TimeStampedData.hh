#pragma once
#include <cstdint>

namespace triggeralgs {
  struct TimeStampedData{
    uint32_t  signal_type    = {0};
    uint64_t  time_stamp     = {0};
    uint32_t  counter	    = {0};
  };
}
