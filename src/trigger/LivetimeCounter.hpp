#ifndef TRIGGER_SRC_TRIGGER_LIVETIMECOUNTER_HPP_
#define TRIGGER_SRC_TRIGGER_LIVETIMECOUNTER_HPP_

#include <chrono>
#include <map>
#include <mutex>
#include <string>

namespace dunedaq::trigger {

class LivetimeCounter
{
public:
  enum class State {
    kLive,
    kDead,
    kPaused
  };

  using state_time_t = uint64_t;
  
  LivetimeCounter(State state);

  ~LivetimeCounter();
  
  void set_state(State state);

  std::map<State, state_time_t> get_time_map();

  state_time_t get_time(State state);

  std::string get_report_string();

  std::string get_state_name(State state) const;
  
private:

  state_time_t now() const;

  // Precondition:  m_mutex is locked by caller
  void update_map();
  
  std::mutex m_mutex;
  State m_state;
  std::map<State, state_time_t> m_state_times;
  state_time_t m_last_state_change_time;
};

} // namespace dunedaq::trigger


#endif // TRIGGER_SRC_TRIGGER_LIVETIMECOUNTER_HPP_
