/**
 * @file TimeSliceOutputBuffer.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
 
#ifndef TRIGGER_SRC_TRIGGER_TIMESLICEOUTPUTBUFFER_HPP_
#define TRIGGER_SRC_TRIGGER_TIMESLICEOUTPUTBUFFERR_HPP_

#include "trigger/Set.hpp"
#include "trigger/Issues.hpp"

#include "logging/Logging.hpp"

#include <vector>
#include <queue>

namespace dunedaq::trigger {

// TODO BJL June 01-2021 would be nice if the T (TriggerPrimative, etc) 
// included a natural ordering with operator<()
template<class T>
struct time_start_greater_t {
    bool operator()(const T &a, const T &b) { 
        return a.time_start > b.time_start; 
    }
};

// When writing Set<T> to a queue, we want to buffer all T with the same for 
// some time, to ensure that Set<T> are generated with all T from that window, 
// assuming that the T may be generated in some arbitrary, but not too tardy, 
// order. Finally, emit Set<T> for completed windows, and warn for any late 
// arriving T.
// This class encapsulates that logic.
template<class T>
class TimeSliceOutputBuffer
{
public:
  // Parameters for ers warning metadata and config
  TimeSliceOutputBuffer(const std::string &name, 
                        const std::string &algorithm, 
                        const dataformats::timestamp_t buffer_time = 0,
                        const dataformats::timestamp_t window_time = 625000) 
    : m_name(name)
    , m_algorithm(algorithm)
    , m_next_window_start(0)
    , m_buffer_time(buffer_time)
    , m_window_time(window_time)
  { }
  
  // Add a new vector<T> to the buffer. 
  void buffer(const std::vector<T> &in)
  {
    if (m_next_window_start == 0) {
      // Window start time is unknown. pick it as the window that contains the
      // first element of in. Window start time must be multiples of m_window_time
      m_next_window_start = (in.front().time_start / m_window_time) * m_window_time;
    }
    for (const T &x : in) {
      if (x.time_start < m_next_window_start) {
        ers::warning(TardyOutputError(ERS_HERE, m_name, m_algorithm));
        // x is discarded
      } else {
        m_buffer.push(x);
        if (m_largest_time < x.time_start) {
          m_largest_time = x.time_start;
        }
      }
    }
  }
  
  void set_window_time(const dataformats::timestamp_t window_time)
  { 
    m_window_time = window_time;
    //next window start must technically be realigned to the new multiple. 
    //this probably never matters, because m_next_window_start is 0 at conf time
    m_next_window_start = (m_next_window_start / m_window_time) * m_window_time;
  } 
  
  // Set the time to wait after a window before a window is emitted in ticks
  void set_buffer_time(const dataformats::timestamp_t buffer_time)
  {
    m_buffer_time = buffer_time;
  }
  
  // True if this buffer has gone m_buffer_time past the end of the first window
  bool ready() {
    if ( empty() ) {
      return false;
    } else {
      return m_largest_time - (m_next_window_start + m_window_time) > m_buffer_time;
    }
  }
  
  bool empty() {
    return m_buffer.empty();
  }
  
  // Fills time_slice, start_time, and end_time with the contents of the buffer
  // that fall within the first window. This removes the contents that are added
  // to time_slice from the buffer, and moves to the next window. Call when 
  // ready() is true for full windows, or whenever to drain this buffer.
  void flush(std::vector<T> &time_slice, 
             dataformats::timestamp_t &start_time, 
             dataformats::timestamp_t &end_time) 
  {
    start_time = m_next_window_start;
    end_time = m_next_window_start + m_window_time;  
    m_next_window_start = m_next_window_start + m_window_time;      
    while (!m_buffer.empty() && m_buffer.top().time_start <= end_time) {
      if (m_buffer.top().time_start < start_time) {
        ers::warning(WindowlessOutputError(ERS_HERE, m_name, m_algorithm));
        // top is discarded
      } else {
        time_slice.emplace_back(m_buffer.top());
      }
      m_buffer.pop();
    }
  }
  
private:
  std::priority_queue<T, std::vector<T>, time_start_greater_t<T> > m_buffer;
  const std::string &m_name, &m_algorithm;
  dataformats::timestamp_t m_next_window_start; // tick start of next window, or 0 if not yet known
  dataformats::timestamp_t m_buffer_time; // ticks to buffer after a window before a window is valid
  dataformats::timestamp_t m_window_time; // width of output windows in ticks
  dataformats::timestamp_t m_largest_time; // larges observed timestamp
};

} // namespace dunedaq::trigger

#endif // TRIGGER_SRC_TRIGGER_TRIGGERGENERICMAKER_HPP_
