#ifndef TRIGGER_INCLUDE_TRIGGER_FRAGMENTMAKERS_HPP_
#define TRIGGER_INCLUDE_TRIGGER_FRAGMENTMAKERS_HPP_

#include "dataformats/Fragment.hpp"
#include "dfmessages/DataRequest.hpp"
#include "trigger/TPSet.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <memory> // For unique_ptr
#include <vector>

namespace dunedaq::trigger {

std::unique_ptr<dataformats::Fragment>
make_fragment(std::vector<TPSet>& tpsets);

std::unique_ptr<dataformats::Fragment>
make_fragment(std::vector<triggeralgs::TriggerPrimitive>& tps);

std::vector<triggeralgs::TriggerPrimitive>
read_fragment_to_trigger_primitives(dataformats::Fragment* frag);

}

#endif // TRIGGER_INCLUDE_TRIGGER_FRAGMENTMAKERS_HPP_
