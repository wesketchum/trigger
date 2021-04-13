
#include "TimingTriggerCandidateMaker.hpp"

#include <chrono>

using internal_clock = std::chrono::duration<double, std::ratio<1, 50'000'000>>;

namespace dunedaq {
namespace trigger {
TimingTriggerCandidateMaker::TimingTriggerCandidateMaker(const std::string& name)
  : DAQModule(name)
  , thread_(std::bind(&TimingTriggerCandidateMaker::do_work, this, std::placeholders::_1))
  , inputQueue_(nullptr)
  , outputQueue_(nullptr)
  , queueTimeout_(100)
{

  register_command("conf", &TimingTriggerCandidateMaker::do_conf);
  register_command("start", &TimingTriggerCandidateMaker::do_start);
  register_command("stop", &TimingTriggerCandidateMaker::do_stop);
  register_command("scrap", &TimingTriggerCandidateMaker::do_scrap);
}

triggeralgs::TriggerCandidate
TimingTriggerCandidateMaker::TimeStampedDataToTriggerCandidate(const triggeralgs::TimeStampedData& data)
{
  // Fill unused fields
  std::vector<uint16_t> detid_list;
  std::vector<triggeralgs::TriggerPrimitive> primitive_list;
  std::vector<triggeralgs::TriggerActivity> activity_list;

  uint32_t detid = data.signal_type;
  triggeralgs::TriggerPrimitive primitive{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  triggeralgs::TriggerActivity activity{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, primitive_list };

  detid_list.push_back(detid);
  primitive_list.push_back(primitive);
  activity_list.push_back(activity);

  triggeralgs::TriggerCandidate candidate;
  auto now = std::chrono::steady_clock::now();

  candidate.time_start = data.time_stamp - m_map[data.signal_type].first,  // time_start
  candidate.time_end = data.time_stamp + m_map[data.signal_type].second, // time_end,
  candidate.time_candidate = data.time_stamp;
  candidate.detid = detid_list;
  candidate.type = TriggerCandidateType::kTiming;
  candidate.algorithm = uint32_t(internal_clock(now.time_since_epoch()).count());
  candidate.version = 0;
  candidate.ta_list = activity_list;

  return candidate;
}

void
TimingTriggerCandidateMaker::do_conf(const nlohmann::json& config)
{
  auto params = config.get<dunedaq::trigger::timingtriggercandidatemaker::Conf>();
  //			try {
  m_map.push_back({ params.s0.time_before, params.s0.time_after });
  m_map.push_back({ params.s1.time_before, params.s1.time_after });
  m_map.push_back({ params.s2.time_before, params.s2.time_after });
  //			} catch(...)  {
  //				ERS_LOG(get_name() << " unsuccessfully configured");
  //			}
  //			ERS_LOG(get_name() << " successfully configured");
}

void
TimingTriggerCandidateMaker::init(const nlohmann::json& iniobj)
{
  auto qi = appfwk::queue_index(iniobj, { "input", "output" });
  // try {
  inputQueue_.reset(new source_t(qi["input"].inst));
  //} catch (const ers::Issue& excpt) {
  //	throw dunedaq::dunetrigger::InvalidQueueFatalError(ERS_HERE, get_name(), "input", excpt);
  //}

  // try {
  outputQueue_.reset(new sink_t(qi["output"].inst));
  //} catch (const ers::Issue& excpt) {
  //	throw dunedaq::dunetrigger::InvalidQueueFatalError(ERS_HERE, get_name(), "output", excpt);
  //}
}

void
TimingTriggerCandidateMaker::do_start(const nlohmann::json&)
{
  thread_.start_working_thread();
  // ERS_LOG(get_name() << " successfully started");
}

void
TimingTriggerCandidateMaker::do_stop(const nlohmann::json&)
{
  thread_.stop_working_thread();
  // ERS_LOG(get_name() << " successfully stopped");
}

void
TimingTriggerCandidateMaker::do_work(std::atomic<bool>& running_flag)
{
  triggeralgs::TimeStampedData data;

  while (running_flag.load()) {
    triggeralgs::TriggerCandidate candidate;

    try {
      inputQueue_->pop(data, queueTimeout_);
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      continue;
    }

    candidate = TimingTriggerCandidateMaker::TimeStampedDataToTriggerCandidate(data);

  // std::string oss_prog = "Activity received #"+std::to_string(receivedCount);
  // ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog));

    bool successfullyWasSent = false;
    while (!successfullyWasSent) {
      // todo: handle timeout exception
      outputQueue_->push(candidate, queueTimeout_);
      successfullyWasSent = true;
    }

    // std::ostringstream oss_summ;
    // oss_summ << ": Exiting do_work() method, received " << receivedCount
    //	 << " TCs and successfully sent " << sentCount << " TCs. ";
    // ers::info(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  }
}

void
TimingTriggerCandidateMaker::do_scrap(const nlohmann::json&)
{}
} // namespace trigger
} // namespace dunedaq
