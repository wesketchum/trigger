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
  if(m_tpset_buffer.size() >= m_buffer_max_size) //delete oldest TPSet if buffer full -> circular buffer
  {
    auto firstIt = m_tpset_buffer.begin();
    m_tpset_buffer.erase(firstIt);
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

  if(start_time > m_buffer_latest_end_time) //condition (3): see end of function
  {
    // add warning here saying data requested hasn't arrived in the buffer yet
    // need to creat a queue of "pending" data request. How?
    return tpsets_output;
  }

  for(auto& tps: m_tpset_buffer)
  {
    if( ( (tps.end_time   > start_time) && (tps.end_time   < end_time) ) ||   //condition (1): see end of function
	( (tps.start_time > start_time) && (tps.start_time < end_time) )    ) //condition (2): see end of function
    {
      tpsets_output.push_back(tps);
    }
  }

  return tpsets_output;

  /*
   Conditions:

   (1) TPSet starts before start_time but finishes before end_time:

            start_time                    end_time
   |--TPSet-------------------------|


   (2) TPSet starts after start_time but finishes after end_time:

            start_time                    end_time
                      |--TPSet-----------------------------|

   (3) TPSet transfer to buffer is delayed

  */

}


} // namespace dunedaq::trigger
