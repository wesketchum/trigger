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

BufferManager::BufferManager()
{

}

BufferManager::~BufferManager()
{

}

bool
BufferManager::add(trigger::TPSet& tps)
{
  return m_tpset_buffer.insert(tps).second; //false if tps with same end_time already exists
}

std::vector<trigger::TPSet>
BufferManager::get_tpsets_in_window(dataformats::timestamp_t start_time, dataformats::timestamp_t end_time)
{
  std::vector<trigger::TPSet> tpsets_output;

  for(auto& tps: m_tpset_buffer)
  {
    if( (tps.start_time > start_time) && (tps.end_time < end_time) )
    {
      tpsets_output.push_back(tps);
      m_tpset_buffer.erase(tps);
    }
  }

  return tpsets_output;


  /*
    Issues that need to be handled:

    1) TPSet starts after start_time but finishes after end_time:

            start_time                    end_time
                      |--TPSet-----------------------------|


    2) TPSet starts before start_time but finishes before end_time:

            start_time                    end_time
   |--TPSet-------------------------|


    3) TPSet transfer to buffer is delayed

  */

}


} // namespace dunedaq::trigger
