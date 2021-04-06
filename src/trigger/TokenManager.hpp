/**
 * @file TimestampEstimator.hpp TimestampEstimator Class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_SRC_TRIGGER_TOKENMANAGER_HPP_
#define TRIGGER_SRC_TRIGGER_TOKENMANAGER_HPP_

#include "appfwk/DAQSource.hpp"

#include "dfmessages/TimeSync.hpp"
#include "dfmessages/TriggerDecisionToken.hpp"
#include "dfmessages/Types.hpp"

#include <atomic>
#include <memory>
#include <set>
#include <thread>

namespace dunedaq {
namespace trigger {

class TokenManager
{
public:
  TokenManager(std::unique_ptr<appfwk::DAQSource<dfmessages::TriggerDecisionToken>>& token_source,
               int initial_tokens,
               dataformats::run_number_t run_number);

  ~TokenManager();

  int get_n_tokens() const;

  bool triggers_allowed() const { return get_n_tokens() > 0; }

  void trigger_sent(dfmessages::trigger_number_t);

private:
  void read_token_queue();

  std::thread m_read_queue_thread;

  std::atomic<bool> m_running_flag;
  std::atomic<int> m_n_tokens;
  int m_n_initial_tokens;
  std::mutex m_open_trigger_decisions_mutex;
  std::set<dfmessages::trigger_number_t> m_open_trigger_decisions;
  std::unique_ptr<appfwk::DAQSource<dfmessages::TriggerDecisionToken>>& m_token_source;
  dataformats::run_number_t m_run_number;
};

}
}

#endif // TRIGGER_SRC_TRIGGER_TOKENMANAGER_HPP_
