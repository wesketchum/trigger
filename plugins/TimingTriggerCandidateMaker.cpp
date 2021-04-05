
#include "trigger/TriggerCandidate.hh"
#include "trigger/TriggerCandidateMaker.hpp"

using clock = std::chrono::duration<double, std::ratio<1, 50'000'000>>;

namespace dunedaq {
	namespace trigger {
		TimingTriggerCandidateMaker::TimingTriggerCandidateMaker(const std::string& name):
			DAQModule(name),
			thread_(std::bind(&TimingTriggerCandidateMaker_Timing::do_work, this, std::placeholders::_1)),
			inputQueue_(nullptr),
			outputQueue_(nullptr),
			queueTimeout_(100) {

			register_command("conf", &TimingTriggerCandidateMaker_Timing::do_conf);
			register_command("start", &TimingTriggerCandidateMaker_Timing::do_start);
			register_command("stop", &TimingTriggerCandidateMaker_Timing::do_stop);
			register_command("scrap", &TimingTriggerCandidateMaker_Timing::do_scrap);
		}


		TriggerCandidate TriggerCandidateMakerTiming::TimeStampedDataToTriggerCandidate(const TimeStampedData& data) {
			//Fill unused fields
			std::vector<uint16_t> detid_list;
			std::vector<TriggerPrimitive> primitive_list;
			std::vector<TriggerActivity> activity_list;

			uint32_t detid = 0;
			TriggerPrimitive primitive {0,0,0,0,0,0,0,0,0,0,0};
			TriggerActivity activity {0,0,0,0,0,0,0,0,0,0,0,0,0,primitive_list};

			detid_list.push_back(detid);
			primitive_list.push_back(primitive);
			activity_list.push_back(activity);

			TriggerCandidate candidate;
			candidate.time_start =  time - m_map[data.signal_type].first, // time_start
			candidate.time_end = time + m_map[data.signal_type].second, // time_end, 
			candidate.time_candidate = int64_t(clock(now.time_since_epoch()).count()); 
			candidate.detid = detid_list;
			candidate.type = data.signal_type;
			candidate.algorithm = data.counter;
			candidate.version = 0;
			candidate.activity_list = activity_list;

			return candidate;
		}

		void TriggerCandidateMakerTiming::do_conf(const nlohmann::json& config) {
			auto params = config.get<dunedaq::trigger::timingtriggercandidatemaker::Conf>();
			try {
				m_map.push({params.s0.time_before,params.s0.time_after});
				m_map.push({params.s1.time_before,params.s1.time_after});
				m_map.push({params.s2.time_before,params.s2.time_after});
			} catch(...)  {
				ERS_LOG(get_name() << " unsuccessfully configured");
			}
			ERS_LOG(get_name() << " successfully configured");
		}

		void TriggerCandidateMakerTiming::init(const nlohmann::json& iniobj) {
			auto qi = appfwk::qindex(iniobj, {"input","output"});
			try {
				inputQueue_.reset(new source_t(qi["input"].inst));
			} catch (const ers::Issue& excpt) {
				throw dunedaq::dunetrigger::InvalidQueueFatalError(ERS_HERE, get_name(), "input", excpt);
			}

			try {
				outputQueue_.reset(new sink_t(qi["output"].inst));
			} catch (const ers::Issue& excpt) {
				throw dunedaq::dunetrigger::InvalidQueueFatalError(ERS_HERE, get_name(), "output", excpt);
			}
		}

		void TriggerCandidateMakerTiming::do_start(const nlohmann::json&) {
			thread_.start_working_thread();
			ERS_LOG(get_name() << " successfully started");
		}

		void TriggerCandidateMakerTiming::do_stop(const nlohmann::json&) {
			thread_.stop_working_thread();
			ERS_LOG(get_name() << " successfully stopped");
		}

		void TriggerCandidateMakerTiming::do_work(std::atomic<bool>& running_flag) {
			int receivedCount = 0;
			int sentCount = 0;
			TimeStampedData data;

			while (running_flag.load()) {
				std::vector<TriggerCandidate> candidates;

				try {
					inputQueue_->pop(data, queueTimeout_);
				} catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
					continue;
				}
				++receivedCount;

				this->operator()(data,candidates);
				candidates.push(TriggerCandidateMakerTiming::TimeStampedDataToTriggerCandidate(data));
				
				//std::string oss_prog = "Activity received #"+std::to_string(receivedCount);
				//ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog));
				
				while(candidates.size()) {
					bool successfullyWasSent = false;
					while (!successfullyWasSent && running_flag.load()) {
						// todo: handle timeout exception
						outputQueue_->push(candidates.back(), queueTimeout_);
						candidates.pop_back();
						successfullyWasSent = true;
						++sentCount;
					}
				}
			}

			std::ostringstream oss_summ;
			oss_summ << ": Exiting do_work() method, received " << receivedCount
				 << " TCs and successfully sent " << sentCount << " TCs. ";
			ers::info(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
		}

		void DAQTriggerCandidateMaker_Timing::do_scrap(const nlohmann::json&) {
		}
	} //namespace trigger
} //namespace dunedaq
