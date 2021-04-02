
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

		void DAQTriggerCandidateMaker::do_conf(const nlohmann::json& config /*args*/) {
			auto params = config.get<dunedaq::triggermodules::daqtriggercandidatemaker::Conf>();
			
			window = params.time_window;
			thresh = params.threshold;
			hit_thresh = params.hit_threshold;
			try {
				m_time_window = {window};
				m_threshold = {thresh};
				m_hit_threshold = {hit_thresh};
			} catch(...)  {
				ERS_LOG(get_name() << " unsuccessfully configured");
			}
			ERS_LOG(get_name() << " successfully configured");
			TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
		}

	} //namespace trigger
}	//namespace dunedaq
