#include "TimingTriggerCandidateMaker.hpp"

namespace dunedaq {
namespace trigger {
TimingTriggerCandidateMaker::TimingTriggerCandidateMaker(const std::string& name)
  : DAQModule(name)
  , m_thread(std::bind(&TimingTriggerCandidateMaker::do_work, this, std::placeholders::_1))
  , m_input_queue(nullptr)
  , m_output_queue(nullptr)
  , m_queue_timeout(100)
{

  register_command("conf", &TimingTriggerCandidateMaker::do_conf);
  register_command("start", &TimingTriggerCandidateMaker::do_start);
  register_command("stop", &TimingTriggerCandidateMaker::do_stop);
  register_command("scrap", &TimingTriggerCandidateMaker::do_scrap);
}

triggeralgs::TriggerCandidate
TimingTriggerCandidateMaker::TimeStampedDataToTriggerCandidate(const triggeralgs::TimeStampedData& data)
{
  triggeralgs::TriggerCandidate candidate;
  if (m_detid_offsets_map.count(data.signal_type)) {
    // clang-format off
    candidate.time_start = data.time_stamp - m_detid_offsets_map[data.signal_type].first;  // time_start
    candidate.time_end   = data.time_stamp + m_detid_offsets_map[data.signal_type].second; // time_end,
    // clang-format on
  } else {
    throw dunedaq::trigger::SignalTypeError(ERS_HERE, get_name(), data.signal_type);
  }
  candidate.time_candidate = data.time_stamp;
  candidate.detid = { static_cast<uint16_t>(data.signal_type) };
  candidate.type = (uint32_t)triggeralgs::TriggerCandidateType::kTiming;
  candidate.algorithm = 0;
  candidate.version = 0;
  candidate.ta_list = {};

  return candidate;
}

void
TimingTriggerCandidateMaker::do_conf(const nlohmann::json& config)
{
  auto params = config.get<dunedaq::trigger::timingtriggercandidatemaker::Conf>();
  m_detid_offsets_map[params.s0.signal_type] = { params.s0.time_before, params.s0.time_after };
  m_detid_offsets_map[params.s1.signal_type] = { params.s1.time_before, params.s1.time_after };
  m_detid_offsets_map[params.s2.signal_type] = { params.s2.time_before, params.s2.time_after };
  TLOG_DEBUG(2) << get_name() + " configured.";
}

void
TimingTriggerCandidateMaker::init(const nlohmann::json& iniobj)
{
  try {
    m_input_queue.reset(new source_t(appfwk::queue_inst(iniobj, "input")));
    m_output_queue.reset(new sink_t(appfwk::queue_inst(iniobj, "output")));
  } catch (const ers::Issue& excpt) {
    throw dunedaq::trigger::InvalidQueueFatalError(ERS_HERE, get_name(), "input/output", excpt);
  }
}

void
TimingTriggerCandidateMaker::do_start(const nlohmann::json&)
{
  m_thread.start_working_thread("timing-tc-maker");
  TLOG_DEBUG(2) << get_name() + " successfully started.";
}

void
TimingTriggerCandidateMaker::do_stop(const nlohmann::json&)
{
  m_thread.stop_working_thread();
  TLOG_DEBUG(2) << get_name() + " successfully stopped.";
}

void
TimingTriggerCandidateMaker::do_work(std::atomic<bool>& running_flag)
{
  size_t n_tsd_received=0;
  size_t n_tc_sent=0;
  
  while (running_flag.load()) {

    triggeralgs::TimeStampedData data;
    try {
      m_input_queue->pop(data, m_queue_timeout);
      ++n_tsd_received;
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      continue;
    }

    triggeralgs::TriggerCandidate candidate;
    try {
      candidate = TimeStampedDataToTriggerCandidate(data);
    } catch (SignalTypeError& e) {
      ers::error(e);
      continue;
    }

    TLOG_DEBUG(2) << "Activity received.";

    bool successfullyWasSent = false;
    while (!successfullyWasSent) {
      try {
        m_output_queue->push(candidate, m_queue_timeout);
        successfullyWasSent = true;
        ++n_tc_sent;
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        std::ostringstream oss_warn;
        oss_warn << "push to output queue \"" << m_output_queue->get_name() << "\"";
        ers::warning(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_queue_timeout.count()));
      }
    }
  }

  TLOG() << "Received " << n_tsd_received << " TimeStampedData messages. Successfully sent " << n_tc_sent << " TriggerCandidates";
  TLOG_DEBUG(2) << "Exiting do_work() method";
}

void
TimingTriggerCandidateMaker::do_scrap(const nlohmann::json&)
{}
} // namespace trigger
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TimingTriggerCandidateMaker)
