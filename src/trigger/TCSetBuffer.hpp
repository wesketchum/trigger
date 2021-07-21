#ifndef TRIGGER_SRC_TCSETBUFFER_HPP_
#define TRIGGER_SRC_TCSETBUFFER_HPP_

#include "trigger/TCSet.hpp"
#include "BufferManager.hpp"

namespace dunedaq::trigger {

  using TCSetBuffer = BufferManager < TCSet >;

} // namespace dunedaq::trigger
#endif
