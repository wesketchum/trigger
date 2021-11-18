/**
 * @file TimingTriggerCandidateMaker.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingTriggerCandidateMaker.hpp"
#include "triggeralgs/Types.hpp"

#include <string>

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
TimingTriggerCandidateMaker::HSIEventToTriggerCandidate(const dfmessages::HSIEvent& data)
{
  triggeralgs::TriggerCandidate candidate;
  // TODO Trigger Team <dune-daq@github.com> Nov-18-2021: the signal field ia now a signal bit map, rather than unique value -> change logic of below?
  if (m_detid_offsets_map.count(data.signal_map)) {
    // clang-format off
    candidate.time_start = data.timestamp - m_detid_offsets_map[data.signal_map].first;  // time_start
    candidate.time_end   = data.timestamp + m_detid_offsets_map[data.signal_map].second; // time_end,
    // clang-format on
  } else {
    throw dunedaq::trigger::SignalTypeError(ERS_HERE, get_name(), data.signal_map);
  }
  candidate.time_candidate = data.timestamp;
  // throw away bits 31-16 of header, that's OK for now
  candidate.detid = { static_cast<triggeralgs::detid_t>(data.header) }; // NOLINT(build/unsigned)
  candidate.type = triggeralgs::TriggerCandidate::Type::kTiming;

  candidate.algorithm = triggeralgs::TriggerCandidate::Algorithm::kHSIEventToTriggerCandidate;
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
  // OpMon.
  m_tsd_received_count.store(0);
  m_tc_sent_count.store(0);
  m_tc_sig_type_err_count.store(0);
  m_tc_total_count.store(0);

  while (true) {

    dfmessages::HSIEvent data;
    try {
      m_input_queue->pop(data, m_queue_timeout);
      ++m_tsd_received_count;
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      // The condition to exit the loop is that we've been stopped and
      // there's nothing left on the input queue
      if (!running_flag.load()) {
        break;
      } else {
        continue;
      }
    }

    triggeralgs::TriggerCandidate candidate;
    try {
      candidate = HSIEventToTriggerCandidate(data);
    } catch (SignalTypeError& e) {
      m_tc_sig_type_err_count++;
      ers::error(e);
      continue;
    }

    TLOG_DEBUG(2) << "Activity received.";

    bool successfullyWasSent = false;
    while (!successfullyWasSent) {
      try {
        m_output_queue->push(candidate, m_queue_timeout);
        successfullyWasSent = true;
        ++m_tc_sent_count;
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        std::ostringstream oss_warn;
        oss_warn << "push to output queue \"" << m_output_queue->get_name() << "\"";
        ers::warning(
          dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_queue_timeout.count()));
      }
    }

    m_tc_total_count++;
  }

  TLOG() << "Received " << m_tsd_received_count << " HSIEvent messages. Successfully sent " << m_tc_sent_count
         << " TriggerCandidates";
  TLOG_DEBUG(2) << "Exiting do_work() method";
}

void
TimingTriggerCandidateMaker::do_scrap(const nlohmann::json&)
{}

void
TimingTriggerCandidateMaker::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  timingtriggercandidatemakerinfo::Info i;

  i.tsd_received_count = m_tsd_received_count.load();
  i.tc_sent_count = m_tc_sent_count.load();
  i.tc_sig_type_err_count = m_tc_sig_type_err_count.load();
  i.tc_total_count = m_tc_total_count.load();

  ci.add(i);
}

} // namespace trigger
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TimingTriggerCandidateMaker)
