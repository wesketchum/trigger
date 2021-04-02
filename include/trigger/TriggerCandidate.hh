#pragma once
#include <cstdint>
#include <vector>
#include "TriggerActivity.hh"

namespace trigger {
  struct TriggerCandidate {
    int64_t  time_start     = {0};
    int64_t  time_end       = {0};
    int64_t  time_candidate = {0};
    std::vector<uint16_t> detid          = {0};
    uint32_t type           = {0};
    uint32_t algorithm      = {0};
    uint16_t version        = {0};

    std::vector<TriggerActivity> activity_list;
  };
    
}
