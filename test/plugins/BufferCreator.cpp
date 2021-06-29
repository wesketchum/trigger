/**
 * @file BufferCreator.cpp BufferCreator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

//#include "CommonIssues.hpp"
#include "BufferCreator.hpp"
#include "trigger/Issues.hpp"
#include "trigger/buffercreator/Nljs.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/app/Nljs.hpp"

#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace trigger {

BufferCreator::BufferCreator(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_thread(std::bind(&BufferCreator::do_work, this, std::placeholders::_1))
  , m_outputQueue()
  , m_queueTimeout(100)
{
  register_command("conf", &BufferCreator::do_configure);
  register_command("start", &BufferCreator::do_start);
  register_command("stop", &BufferCreator::do_stop);
  register_command("scrap", &BufferCreator::do_scrap);
}

void
BufferCreator::init(const nlohmann::json& init_data)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";

  m_outputQueue.reset(new appfwk::DAQSink<dfmessages::HSIEvent>(appfwk::queue_inst(init_data, "hsievent_sink")));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
BufferCreator::get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/)
{}

void
BufferCreator::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";

  auto params = obj.get<buffercreator::Conf>();

  m_sleep_time = params.sleep_time;
  m_buffer_size = params.buffer_size;

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
BufferCreator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_thread.start_working_thread("buffer-man");
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
BufferCreator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_thread.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
BufferCreator::do_scrap(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}

void
BufferCreator::do_work(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
  size_t generatedCount = 0;
  size_t sentCount = 0;

  auto start_time = std::chrono::steady_clock::now();
  auto period = std::chrono::nanoseconds(m_sleep_time);
  auto next_time_step = start_time + period;

  while (running_flag.load()) {
    TLOG_DEBUG(TLVL_GENERATION) << get_name() << ": Start of sleep between sends";

    std::this_thread::sleep_until(next_time_step);

    next_time_step += period;

    //dfmessages::HSIEvent tsd = get_hsievent();

    generatedCount++;

    std::string thisQueueName = m_outputQueue->get_name();
    bool successfullyWasSent = false;
    // do...while instead of while... so that we always try at least
    // once to send everything we generate, even if running_flag is
    // changed to false between the top of the main loop and here
    do {
      TLOG_DEBUG(TLVL_GENERATION) << get_name() << ": Pushing the generated TSD onto queue " << thisQueueName;
      try {
        //m_outputQueue->push(tsd, m_queueTimeout);
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

  TLOG() << ": Exiting the do_work() method, generated " << generatedCount << " buffer calls.";

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace trigger
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::BufferCreator)

// Local Variables:
// c-basic-offset: 2
// End:
