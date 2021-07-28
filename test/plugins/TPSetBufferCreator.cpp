/**
 * @file TPSetBufferCreator.cpp TPSetBufferCreator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

//#include "CommonIssues.hpp"
#include "TPSetBufferCreator.hpp"
#include "trigger/Issues.hpp"
#include "trigger/tpsetbuffercreator/Nljs.hpp"

#include "dataformats/FragmentHeader.hpp"
#include "dataformats/GeoID.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/app/Nljs.hpp"

#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace trigger {

TPSetBufferCreator::TPSetBufferCreator(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_thread(std::bind(&TPSetBufferCreator::do_work, this, std::placeholders::_1))
  , m_queueTimeout(100)
  , m_input_queue_tps()
  , m_input_queue_dr()
  , m_output_queue_frag()
  , m_tps_buffer()
{
  register_command("conf", &TPSetBufferCreator::do_configure);
  register_command("start", &TPSetBufferCreator::do_start);
  register_command("stop", &TPSetBufferCreator::do_stop);
  register_command("scrap", &TPSetBufferCreator::do_scrap);
}

void
TPSetBufferCreator::init(const nlohmann::json& init_data)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";

  m_input_queue_tps.reset(new tps_source_t(appfwk::queue_inst(init_data, "tpset_source")));
  m_input_queue_dr.reset(new dr_source_t(appfwk::queue_inst(init_data, "data_request_source")));
  m_output_queue_frag.reset(new fragment_sink_t(appfwk::queue_inst(init_data, "fragment_sink")));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
TPSetBufferCreator::get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/)
{}

void
TPSetBufferCreator::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";

  auto params = obj.get<tpsetbuffercreator::Conf>();

  m_tps_buffer_size = params.tpset_buffer_size;

  m_tps_buffer.reset(new TPSetBuffer(m_tps_buffer_size));

  m_tps_buffer->set_buffer_size(m_tps_buffer_size);

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
TPSetBufferCreator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_thread.start_working_thread("buffer-man");
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
TPSetBufferCreator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_thread.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
TPSetBufferCreator::do_scrap(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  m_tps_buffer.reset(nullptr); // calls dtor
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}

std::unique_ptr<dataformats::Fragment>
TPSetBufferCreator::convert_to_fragment(TPSetBuffer::data_request_output ds_output, dfmessages::DataRequest input_data_request)
{
  auto ret=std::make_unique<dataformats::Fragment>(std::vector<std::pair<void*, size_t>>());
  auto& frag=*ret.get();

  dataformats::GeoID          geoid;
  dataformats::FragmentHeader frag_h;
  frag_h.trigger_number    = input_data_request.trigger_number;
  frag_h.trigger_timestamp = input_data_request.trigger_timestamp;
  frag_h.window_begin      = input_data_request.window_begin;
  frag_h.window_end        = input_data_request.window_end;
  frag_h.run_number        = input_data_request.run_number;
  frag_h.element_id        = geoid;
  frag_h.error_bits        = 0;
  frag_h.fragment_type     = 0;
  //frag_h.sequence_number = input_data_request.sequence_number; -->compilation error. sequence_number still only on develop branch

  frag.set_header_fields(frag_h);

  /*  >>> need to fill the fragment with
      the contents of the TPSet vector here
   */

  return ret;
}

void
TPSetBufferCreator::send_out_fragment(std::unique_ptr<dataformats::Fragment> frag_out, size_t& sentCount, std::atomic<bool>& running_flag)
{
  std::string thisQueueName = m_output_queue_frag->get_name();
  bool successfullyWasSent = false;
  // do...while so that we always try at least once to send
  // everything we generate, even if running_flag is changed
  // to false between the top of the main loop and here
  do {
    TLOG_DEBUG(TLVL_GENERATION) << get_name() << ": Pushing the requested TPSet onto queue " << thisQueueName;
    try {
      m_output_queue_frag->push(std::move(frag_out), m_queueTimeout);
      successfullyWasSent = true;
      ++sentCount;
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      std::ostringstream oss_warn;
      oss_warn << "push to output queue \"" << thisQueueName << "\"";
      ers::warning(
		   dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_queueTimeout.count()));
    }
  } while (!successfullyWasSent && running_flag.load());
}

void
TPSetBufferCreator::do_work(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
  size_t addedCount = 0;
  size_t requestedCount = 0;
  size_t sentCount = 0;

  bool first=true;

  while (running_flag.load()) {

    trigger::TPSet input_tpset;
    dfmessages::DataRequest input_data_request;
    TPSetBuffer::data_request_output requested_tpset;

    // Block that receives TPSets and add them in buffer and check for pending data requests
    try {
      m_input_queue_tps->pop(input_tpset, m_queueTimeout);
      if(first){
        TLOG() << "Got first TPSet, with start_time=" << input_tpset.start_time;
        first=false;
      }

      m_tps_buffer->add(input_tpset);
      ++addedCount;

      if( m_dr_on_hold.size() ){ //check if new data is part of data request on hold

	std::map<dfmessages::DataRequest, std::vector<trigger::TPSet>>::iterator it = m_dr_on_hold.begin();

	while(it != m_dr_on_hold.end()){

	  if( it->first.window_begin < input_tpset.end_time || it->first.window_end > input_tpset.start_time ){ //new tpset is whithin data request windown?
	    it->second.push_back(input_tpset);

	    if( it->first.window_end < input_tpset.end_time ){ //If more TPSet aren't expected to arrive then push
	      requested_tpset.txsets_in_window = it->second;
              std::unique_ptr<dataformats::Fragment> frag_out = convert_to_fragment(requested_tpset,it->first);
	      send_out_fragment(std::move(frag_out), sentCount, running_flag);
	      m_dr_on_hold.erase(it);
	      it--;
	      continue;
	    }
	  }
	  it++;
	}

      }
      if( m_dr_on_hold.size() ){ // if there are still requests pending, wait for more TPSets to be added in buffer.
	continue;
      }

    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt)
      { }

    // Block that reveives data requests and return fragments from buffer
    try {
      m_input_queue_dr->pop(input_data_request, m_queueTimeout);
      requested_tpset = m_tps_buffer->get_txsets_in_window( input_data_request.window_begin, input_data_request.window_end );
      ++requestedCount;

      auto frag_out = convert_to_fragment(requested_tpset, input_data_request);

      switch(requested_tpset.ds_outcome) {
        case TPSetBuffer::kEmpty:
	  TLOG() << get_name() << " Buffer does not contain data requested. Returning empty fragment.";
	  send_out_fragment(std::move(frag_out), sentCount, running_flag);
	  break;
        case TPSetBuffer::kLate:
	  TLOG() << get_name() << " Requested data (" << input_data_request.window_begin << ", " << input_data_request.window_end << ") has not arrived in buffer, which contains (" << m_tps_buffer->get_earliest_start_time() << ", " << m_tps_buffer->get_latest_end_time() << "). Holding request until more data arrives.";
	  m_dr_on_hold.insert(std::make_pair(input_data_request, requested_tpset.txsets_in_window));
	  break; // don't send anything yet. Wait for more data to arrived.
        case TPSetBuffer::kSuccess:
	  TLOG_DEBUG(0) << get_name() << "Sending requested data.";
	  send_out_fragment(std::move(frag_out), sentCount, running_flag);
	  break;
        default :
	  TLOG() << get_name() << " Data request failed!";
      }

    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      // skip if no data request in the queue
      continue;
    }

  }

  TLOG() << ": Exiting the do_work() method: received " << addedCount << " TPs and " << requestedCount << " data requests. Sent " << sentCount << " fragments";

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace trigger
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TPSetBufferCreator)

// Local Variables:
// c-basic-offset: 2
// End:
