/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_TEST_PLUGINS_BUFFERCREATOR_HPP_
#define TRIGGER_TEST_PLUGINS_BUFFERCREATOR_HPP_

#include "dataformats/Types.hpp"
#include "dataformats/Fragment.hpp"

#include "dfmessages/HSIEvent.hpp"
#include "dfmessages/DataRequest.hpp"

#include "trigger/buffercreator/Structs.hpp"
#include "trigger/TPSet.hpp"
#include "trigger/BufferManager.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include <ers/Issue.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace dunedaq {
namespace trigger {

/**
 * @brief BufferCreator creates a buffer that stores TPSets and handles data requests.
 */
class BufferCreator : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief BufferCreator Constructor
   * @param name Instance name for this BufferCreator instance
   */
  explicit BufferCreator(const std::string& name);

  BufferCreator(const BufferCreator&) =
    delete; ///< BufferCreator is not copy-constructible
  BufferCreator& operator=(const BufferCreator&) =
    delete; ///< BufferCreator is not copy-assignable
  BufferCreator(BufferCreator&&) =
    delete; ///< BufferCreator is not move-constructible
  BufferCreator& operator=(BufferCreator&&) =
    delete; ///< BufferCreator is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_scrap(const nlohmann::json& obj);

  // Threading
  dunedaq::appfwk::ThreadHelper m_thread;
  void do_work(std::atomic<bool>&);

  // Configuration
  std::chrono::milliseconds m_queueTimeout;

  using tps_source_t = dunedaq::appfwk::DAQSource<trigger::TPSet>;
  std::unique_ptr<tps_source_t> m_input_queue_tps;

  using dr_source_t = dunedaq::appfwk::DAQSource<dfmessages::DataRequest>;
  std::unique_ptr<dr_source_t> m_input_queue_dr;

  using fragment_sink_t = dunedaq::appfwk::DAQSink<dataformats::Fragment>;
  std::unique_ptr<fragment_sink_t> m_output_queue_frag;

  trigger::BufferManager* m_buffer;

  uint64_t m_buffer_size;

  struct DataRequestComp{
    bool operator()(const dfmessages::DataRequest& left, const dfmessages::DataRequest& right) const{
      return left.window_begin < right.window_end;
    }
  };

  std::map<dfmessages::DataRequest, std::vector<trigger::TPSet>, DataRequestComp>* m_dr_on_hold; ///< Holds data request when data has not arrived in the buffer yet

  dataformats::Fragment convert_to_fragment(BufferManager::data_request_output);

  void send_out_fragment(BufferManager::data_request_output, size_t&, std::atomic<bool>&);

};
} // namespace trigger
} // namespace dunedaq

#endif // TRIGGER_TEST_PLUGINS_BUFFERCREATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
