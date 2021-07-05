/**
 * @file BufferManager.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "trigger/BufferManager.hpp"

#include <iostream>

namespace dunedaq::trigger {

BufferManager::BufferManager(long unsigned int buffer_size)
  : m_buffer_max_size(buffer_size)
  , m_buffer_earliest_start_time(0)
  , m_buffer_latest_end_time(0)
{

}

BufferManager::~BufferManager()
{

}

bool
BufferManager::add(trigger::TPSet& tps)
{
  if(m_tpset_buffer.size() >= m_buffer_max_size) //delete oldest TPSet if buffer full (and updating earliest start time) -> circular buffer
  {
    auto firstIt = m_tpset_buffer.begin();
    m_tpset_buffer.erase(firstIt);
    firstIt++;
    m_buffer_earliest_start_time = (*firstIt).start_time;
  }
  if( (m_buffer_earliest_start_time == 0) || (tps.start_time < m_buffer_earliest_start_time) )
    m_buffer_earliest_start_time = tps.start_time;
  
  if( (m_buffer_latest_end_time == 0) || (tps.end_time > m_buffer_latest_end_time) )
    m_buffer_latest_end_time = tps.end_time;

  return m_tpset_buffer.insert(tps).second; //false if tps with same start_time already exists
}

std::vector<trigger::TPSet>
BufferManager::get_tpsets_in_window(dataformats::timestamp_t start_time, dataformats::timestamp_t end_time)
{
  std::vector<trigger::TPSet> tpsets_output;

  if(end_time < m_buffer_earliest_start_time)
  {
    // add warning here saying data requested doesn't exist in the buffer anymore
    return tpsets_output;
  }

  if(start_time > m_buffer_latest_end_time)
  {
    // add warning here saying data requested hasn't arrived in the buffer yet
    // need to create a queue of "pending" data request. How?
    return tpsets_output;
  }

  trigger::TPSet tpset_low, tpset_up;
  tpset_low.start_time = start_time;
  tpset_up.start_time  = end_time;

  std::set<trigger::TPSet,TPSetCmp>::iterator it, it_low,it_up;

  //checking first and last TPSet of buffer that have a start_time within data request limits
  it_low = m_tpset_buffer.lower_bound(tpset_low);
  it_up  = m_tpset_buffer.upper_bound(tpset_up);
  it     = it_low;

  //checking if previous TPset has a end_time that is after the data request's start time
  it--;
  if((*it).end_time > start_time) tpsets_output.push_back(*it);

  //loading TPSets
  it++;
  while(it != it_up){
    tpsets_output.push_back(*it);
    it++;
  }

  return tpsets_output;

}

} // namespace dunedaq::trigger
