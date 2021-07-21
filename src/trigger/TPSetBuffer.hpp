#ifndef TRIGGER_SRC_TPSETBUFFER_HPP_
#define TRIGGER_SRC_TPSETBUFFER_HPP_

#include "trigger/TPSet.hpp"
#include "BufferManager.hpp"

namespace dunedaq::trigger {

  using TPSetBuffer = BufferManager < TPSet >;

} // namespace dunedaq::trigger
#endif
