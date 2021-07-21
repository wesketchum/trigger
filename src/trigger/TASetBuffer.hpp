#ifndef TRIGGER_SRC_TASETBUFFER_HPP_
#define TRIGGER_SRC_TASETBUFFER_HPP_

#include "trigger/TASet.hpp"
#include "BufferManager.hpp"

namespace dunedaq::trigger {

  using TASetBuffer = BufferManager < TASet >;

} // namespace dunedaq::trigger
#endif
