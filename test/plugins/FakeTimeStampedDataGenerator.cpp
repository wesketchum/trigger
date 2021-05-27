/**
 * @file FakeTimeStampedDataGenerator.cpp FakeTimeStampedDataGenerator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

//#include "CommonIssues.hpp"
#include "FakeTimeStampedDataGenerator.hpp"
#include "trigger/Issues.hpp"
#include "trigger/faketimestampeddatagenerator/Nljs.hpp"

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

FakeTimeStampedDataGenerator::FakeTimeStampedDataGenerator(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_thread(std::bind(&FakeTimeStampedDataGenerator::do_work, this, std::placeholders::_1))
  , m_outputQueue()
  , m_queueTimeout(100)
  , m_generator()
  , m_counts(0)
{
  register_command("conf", &FakeTimeStampedDataGenerator::do_configure);
  register_command("start", &FakeTimeStampedDataGenerator::do_start);
  register_command("stop", &FakeTimeStampedDataGenerator::do_stop);
  register_command("scrap", &FakeTimeStampedDataGenerator::do_scrap);
}

void
FakeTimeStampedDataGenerator::init(const nlohmann::json& init_data)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";

  m_outputQueue.reset(new appfwk::DAQSink<dfmessages::HSIEvent>(appfwk::queue_inst(init_data, "hsievent_sink")));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
FakeTimeStampedDataGenerator::get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/)
{}

void
FakeTimeStampedDataGenerator::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";

  auto params = obj.get<faketimestampeddatagenerator::Conf>();

  m_sleep_time = params.sleep_time;
  m_frequency = params.frequency;

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
FakeTimeStampedDataGenerator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_thread.start_working_thread("fake-tsd-gen");
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
FakeTimeStampedDataGenerator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_thread.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
FakeTimeStampedDataGenerator::do_scrap(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}

dfmessages::HSIEvent
FakeTimeStampedDataGenerator::get_hsievent()
{
  dfmessages::HSIEvent tsd{};

  int signaltype = m_rdm_signaltype(m_generator);

  auto tsd_start_time = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::ratio<1, 50000000>> elapsed_time = tsd_start_time.time_since_epoch();

  // leave header as 0
  tsd.timestamp = (static_cast<uint64_t>(elapsed_time.count())) * m_frequency / 50000000; // NOLINT(build/unsigned)
  tsd.signal_map = signaltype;
  tsd.sequence_counter = m_counts;
  m_counts++;

  TLOG_DEBUG(TLVL_GENERATION) << get_name() << tsd.timestamp << ", " << tsd.signal_map << ", " << tsd.sequence_counter
                              << "\n";

  return tsd;
}

void
FakeTimeStampedDataGenerator::do_work(std::atomic<bool>& running_flag)
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

    dfmessages::HSIEvent tsd = get_hsievent();

    generatedCount++;

    std::string thisQueueName = m_outputQueue->get_name();
    bool successfullyWasSent = false;
    // do...while instead of while... so that we always try at least
    // once to send everything we generate, even if running_flag is
    // changed to false between the top of the main loop and here
    do {
      TLOG_DEBUG(TLVL_GENERATION) << get_name() << ": Pushing the generated TSD onto queue " << thisQueueName;
      try {
        m_outputQueue->push(tsd, m_queueTimeout);
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

  auto end_time = std::chrono::steady_clock::now();
  using doublesec = std::chrono::duration<double, std::ratio<1>>;
  double khz = 1e-3 * generatedCount / doublesec(end_time - start_time).count();
  std::ostringstream oss_summ;
  oss_summ << ": Exiting the do_work() method, generated " << generatedCount << " TSD set and successfully sent "
           << sentCount << " copies. " << khz << "kHz";
  ers::info(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace trigger
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::FakeTimeStampedDataGenerator)

// Local Variables:
// c-basic-offset: 2
// End:
