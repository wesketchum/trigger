#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include "CommonIssues.hpp"

#include "TriggerCandidateType.hh"
#include "trigger/timingtriggercandidatemaker/Nljs.hpp"

#include "dune-trigger-algs/TimeStampedData.hh"
#include "dune-trigger-algs/TriggerActivity.hh"
#include "dune-trigger-algs/TriggerCandidate.hh"
#include "dune-trigger-algs/TriggerPrimitive.hh"


#include <chrono>

namespace dunedaq {
namespace trigger {
class TimingTriggerCandidateMaker : public dunedaq::appfwk::DAQModule
{
public:
  explicit TimingTriggerCandidateMaker(const std::string& name);

  TimingTriggerCandidateMaker(const TimingTriggerCandidateMaker&) = delete;
  TimingTriggerCandidateMaker& operator=(const TimingTriggerCandidateMaker&) = delete;
  TimingTriggerCandidateMaker(TimingTriggerCandidateMaker&&) = delete;
  TimingTriggerCandidateMaker& operator=(TimingTriggerCandidateMaker&&) = delete;

  void init(const nlohmann::json& iniobj) override;

private:
  void do_conf(const nlohmann::json& config);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_scrap(const nlohmann::json& obj);

  dunedaq::appfwk::ThreadHelper thread_;

  triggeralgs::TriggerCandidate TimeStampedDataToTriggerCandidate(const triggeralgs::TimeStampedData& data);
  void do_work(std::atomic<bool>&);

  using source_t = dunedaq::appfwk::DAQSource<triggeralgs::TimeStampedData>;
  std::unique_ptr<source_t> inputQueue_;

  using sink_t = dunedaq::appfwk::DAQSink<triggeralgs::TriggerCandidate>;
  std::unique_ptr<sink_t> outputQueue_;

  std::chrono::milliseconds queueTimeout_;

  std::vector<std::pair<int64_t, int64_t>> m_detid_offsets_map;
};
} // namespace trigger
} // namespace dunedaq
