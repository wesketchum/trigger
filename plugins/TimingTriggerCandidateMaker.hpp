#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include "trigger/timingtriggercandidatemaker/Nljs.hpp"
#include "trigger/TimeStampedData.hh"
#include "trigger/TriggerCandidate.hh"

#include <chrono>

namespace dunedaq {
	namespace trigger {
		class TimingTriggerCandidateMaker: public dunedaq::appfwk::DAQModule, TriggerCandidateMakerTiming {
		public:
			explicit TimingTriggerCandidateMaker(const std::string& name);

			TimingTriggerCandidateMaker(const TimingTriggerCandidateMaker&) = delete;
			TimingTriggerCandidateMaker& operator=(const TimingTriggerCandidateMaker&) = delete;
			TimingTriggerCandidateMaker(TimingTriggerCandidateMaker&&) = delete;
			TimingTriggerCandidateMaker& operator=(TimingTriggerCandidateMaker&&) = delete;

			void init(const nlohmann::json& obj) override;
      
		private:
			void do_conf(const nlohmann::json& obj);
			void do_start(const nlohmann::json& obj);
			void do_stop(const nlohmann::json& obj);
			void do_scrap(const nlohmann::json& obj);

			dunedaq::appfwk::ThreadHelper thread_;

			TriggerCandidate TriggerCandidateMakerTiming::TimeStampedDataToTriggerCandidate(const TimeStampedData& data);
			void do_work(std::atomic<bool>&);

			using source_t = dunedaq::appfwk::DAQSource<TimeStampedData>;
			std::unique_ptr<source_t> inputQueue_;

			using sink_t = dunedaq::appfwk::DAQSink<TriggerCandidate>;
			std::unique_ptr<sink_t> outputQueue_;

			std::chrono::milliseconds queueTimeout_;
		};
	} //namespace trigger
} //namespace dunedaq
