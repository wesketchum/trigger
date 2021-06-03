/**
 * @file BufferManager.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_SRC_TRIGGER_BUFFERMANAGER_HPP_
#define TRIGGER_SRC_TRIGGER_BUFFERMANAGER_HPP_

#include "dataformats/Types.hpp"
#include "trigger/TPSet.hpp"

#include <set>

namespace dunedaq {
namespace trigger {

/**
 * @brief BufferManager description.
 *
 */
class BufferManager
{
public:
  BufferManager();

  virtual ~BufferManager();

  BufferManager(BufferManager const&) = delete;
  BufferManager(BufferManager&&) = default;
  BufferManager& operator=(BufferManager const&) = delete;
  BufferManager& operator=(BufferManager&&) = default;

  /**
   *  add a TPSet to the buffer. Remove oldest TPSets from buffer if we are at maximum size
   */
  bool add(trigger::TPSet& tps);
  
  /**
   * return a vector of all the TPSets in the buffer that overlap with [start_time, end_time]
   */
  bool get_tpsets_in_window(dataformats::timestamp_t start_time, dataformats::timestamp_t end_time);

private:
  //Where the TPSet will be buffered
  std::set<trigger::TPSet> m_tpset_buffer;

  //Vector with TPSset for the get function
  std::vector<trigger::TPSet> m_requested_tpset;

};

} // namespace trigger
} // namespace dunedaq

#endif // TRIGGER_SRC_TRIGGER_BUFFERMANAGER_HPP_
