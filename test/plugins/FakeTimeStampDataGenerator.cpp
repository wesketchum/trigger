/**
 * @file FakeTimeStampDataGenerator.cpp FakeTimeStampDataGenerator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "CommonIssues.hpp"
#include "FakeTimeStampDataGenerator.hpp"

#include "appfwk/app/Nljs.hpp"

#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <thread>
#include <string>
#include <vector>

using pd_clock = std::chrono::duration<double, std::ratio<1, 50000000>>;

namespace dunedaq {
namespace trigger {

FakeTimeStampDataGenerator::FakeTimeStampDataGenerator(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , thread_(std::bind(&FakeTimeStampDataGenerator::do_work, this, std::placeholders::_1))
  , outputQueue_()
  , queueTimeout_(100)
  , generator()
{
  register_command("conf",  &FakeTimeStampDataGenerator::do_configure);
  register_command("start", &FakeTimeStampDataGenerator::do_start);
  register_command("stop",  &FakeTimeStampDataGenerator::do_stop);
  register_command("scrap", &FakeTimeStampDataGenerator::do_scrap);
}

void
FakeTimeStampDataGenerator::init(const nlohmann::json& init_data)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto ini = init_data.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.dir != "output") {
      continue;                 // skip all but "output" direction
    }
    try
    {
      //outputQueue_.emplace_back(new sink_t(qi.inst));
      outputQueue_.reset(new sink_t(qi.inst));
    }
    catch (const ers::Issue& excpt)
    {
      throw dunedaq::dunetrigger::InvalidQueueFatalError(ERS_HERE, get_name(), qi.name, excpt);
    }
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
FakeTimeStampDataGenerator::get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/)
{
}

void
FakeTimeStampDataGenerator::do_configure(const nlohmann::json& /*obj*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
FakeTimeStampDataGenerator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  thread_.start_working_thread();
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
FakeTimeStampDataGenerator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  thread_.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
FakeTimeStampDataGenerator::do_scrap(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}

 std::vector<triggeralgs::TimeStampedData>
FakeTimeStampDataGenerator::GetTimestamp()
{
  std::vector<triggeralgs::TimeStampedData> tsds;
  triggeralgs::TimeStampedData tsd{};

  int signaltype = rdm_signaltype(generator);

  auto tsd_start_time = std::chrono::steady_clock::now();
  tsd.time_stamp = (uint64_t)pd_clock(tsd_start_time.time_since_epoch()).count();
  tsd.signal_type = signaltype;
  auto now = std::chrono::steady_clock::now();
  tsd.counter = (uint32_t)pd_clock(now.time_since_epoch()).count();

  //std::cout << "\033[32mtsd.timestamp: " << tsd.time_stamp << "\033[0m  ";
  std::cout << "\033[32m" << tsd.time_stamp << ", "<< tsd.signal_type << ", "<< tsd.counter << "\033[0m\n";

  tsds.push_back(tsd);
  
  return tsds;
}

void
FakeTimeStampDataGenerator::do_work(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
  size_t generatedCount = 0;
  size_t sentCount = 0;

  while (running_flag.load()) 
  {
    TLOG_DEBUG(TLVL_GENERATION) << get_name() << ": Start of sleep between sends";
    std::this_thread::sleep_for(std::chrono::nanoseconds(1000000000));

    std::vector<triggeralgs::TimeStampedData> tsds = GetTimestamp();

    if (tsds.size() == 0) 
    {
      std::ostringstream oss_prog;
      oss_prog << "Last TSDs packet has size 0, continuing!";
      ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));
      continue; 
    } 
    else 
    {
      std::ostringstream oss_prog;
      ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));
    }

    generatedCount+=tsds.size();

    std::string thisQueueName = outputQueue_->get_name();
    bool successfullyWasSent = false;
    while (!successfullyWasSent && running_flag.load())
    {
      TLOG_DEBUG(TLVL_GENERATION) << get_name() << ": Pushing the generated TSD onto queue " << thisQueueName;
      for (auto const& tsd: tsds)
      {
	try
	{
	  outputQueue_->push(tsd, queueTimeout_);
	  successfullyWasSent = true;
	  ++sentCount;
	}
	catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt)
	{
	  std::ostringstream oss_warn;
	  oss_warn << "push to output queue \"" << thisQueueName << "\"";
	  ers::warning(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(),
							    std::chrono::duration_cast<std::chrono::milliseconds>(queueTimeout_).count()));
	}
      }
    }

  }

  std::ostringstream oss_summ;
  oss_summ << ": Exiting the do_work() method, generated " << generatedCount
           << " TSD set and successfully sent " << sentCount << " copies. ";
  ers::info(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace trigger 
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::FakeTimeStampDataGenerator)

// Local Variables:
// c-basic-offset: 2
// End:
