#include "appfwk/ThreadHelper.hpp"
#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"

#include "TriggerPrimitiveMaker.hpp"

#include "appfwk/cmd/Nljs.hpp"

#include <chrono>
#include <string>
#include <vector>
#include <fstream>

#include "trigger/AlgorithmPlugins.hpp"
//#include "trigger/triggerprimitivemaker/Nljs.hpp"

using pd_clock = std::chrono::duration<double, std::ratio<1, 50000000>>;
using namespace triggeralgs;

namespace dunedaq::trigger {

    TriggerPrimitiveMaker::TriggerPrimitiveMaker(const std::string& name) :
      dunedaq::appfwk::DAQModule(name),
      thread_(std::bind(&TriggerPrimitiveMaker::do_work, this, std::placeholders::_1)),
      outputQueue_(),
      queueTimeout_(100)
      {
      register_command("conf"       , &TriggerPrimitiveMaker::do_configure  );
      register_command("start"      , &TriggerPrimitiveMaker::do_start      );
      register_command("stop"       , &TriggerPrimitiveMaker::do_stop       );
      register_command("scrap"      , &TriggerPrimitiveMaker::do_unconfigure);
    }

    std::vector<std::vector<int64_t>> TriggerPrimitiveMaker::ReadCSV(const std::string filename) {
	std::vector<std::vector<int64_t>> tps_vector;
        std::ifstream src(filename);
        if(!src.is_open()) throw std::runtime_error("Could not open file");
        int64_t val;

        std::string buffer;
        char sep = ',';
	int rowIdx = 0;
        while (std::getline(src, buffer))
            {
            tps_vector.push_back({0,0,0,0,0,0,0,0});
            std::stringstream ss(buffer);
            int colIdx = 0;
            while(ss >> val)
            {
                tps_vector.at(rowIdx)[colIdx] = val;
                if(ss.peek() == sep) ss.ignore();
                colIdx++;
            }
            rowIdx++;
        }
        src.close();
	return tps_vector;
     }

    /*void TriggerPrimitiveMaker::init(const nlohmann::json& init_data) {
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
      auto ini = init_data.get<appfwk::cmd::ModInit>();
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
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
    }

    void TriggerPrimitiveMaker::do_configure(const nlohmann::json& config ) {
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";
      auto params = config.get<dunedaq::triggermodules::triggerprimitivefromfile::Conf>();
      filename = params.filename;

      // Check if file is loaded
      // std::ifstream src(filename);
      // if(!src.is_open()) throw InvalidConfiguration(ERS_HERE);
	      // src.close();
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
    }*/

    void TriggerPrimitiveMaker::do_start(const nlohmann::json& /*args*/) {
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
      thread_.start_working_thread();
      //ERS_LOG(get_name() << " successfully started");
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
    }

    void TriggerPrimitiveMaker::do_stop(const nlohmann::json& /*args*/) {
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
      thread_.stop_working_thread();
      //ERS_LOG(get_name() << " successfully stopped");
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
    }

    void TriggerPrimitiveMaker::do_unconfigure(const nlohmann::json& /*args*/) {
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_unconfigure() method";
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_unconfigure() method";
    }

    std::vector<TPSet> TriggerPrimitiveMaker::GetEvts(std::vector<std::vector<int64_t>> tps_vector) {
        std::cout << "\033[28m ENTERING TP GENERATOR WITH SOURCE FILE " << filename << "\033[0m  ";
        std::cout << "\033[28m TPs vector size: " << tps_vector.size() << "\033[0m  ";
      std::vector<TriggerPrimitive> tps;
      int EvtNo = tps_vector.size();
      for (int i=0; i<EvtNo; ++i) {
        TriggerPrimitive tp{};

        tp.time_start          = (int64_t)tps_vector[i][0];
        std::cout << "\033[31mtp.time_start : " << tp.time_start << "\033[0m  ";
        tp.time_over_threshold = (int64_t)tps_vector[i][1];
        tp.time_peak           = (int64_t)tps_vector[i][2];
        tp.channel             = (uint16_t)tps_vector[i][3];
        std::cout << "\033[32mtp.channel : " << tp.channel << "\033[0m\n";
        tp.adc_integral        = (uint32_t)tps_vector[i][4];
        tp.adc_peak            = (uint16_t)tps_vector[i][5];
        tp.detid               = (uint16_t)tps_vector[i][6];
        tp.type		       = (uint16_t)tps_vector[i][7];
        auto now = std::chrono::steady_clock::now();
        tp.flag = (uint32_t)pd_clock(now.time_since_epoch()).count();
          std::cout << "\033[31mTimestamp : "     << tp.algorithm<< "\033[0m  ";
        tps.push_back(tp);
      }
      TPSet tpset_empty;
      std::vector<TPSet> tpset_empty_vector;
      tpset_empty_vector.push_back(tpset_empty);
      return tpset_empty_vector;
    }
    
    void TriggerPrimitiveMaker::do_work(std::atomic<bool>& running_flag) {
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
      size_t generatedCount = 0;
      size_t sentCount = 0;

      while (running_flag.load()) {
        TLOG(TLVL_GENERATION) << get_name() << ": Start of sleep between sends";
        std::this_thread::sleep_for(std::chrono::nanoseconds(1000000000));

	
        std::vector<std::vector<int64_t>> output_vector = ReadCSV(filename);
        std::vector<TPSet> tps = GetEvts(output_vector);

        if (tps.size() == 0) {
          std::ostringstream oss_prog;
          oss_prog << "Last TPs packet has size 0, continuing!";
          //ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));
          continue; 
        } else {
          std::ostringstream oss_prog;
          oss_prog << "Generated TPs #" << generatedCount << " last TPs packet has size " << tps.size();
          //ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));
        }

        generatedCount+=tps.size();
        
        std::string thisQueueName = outputQueue_->get_name();
        TLOG(TLVL_GENERATION) << get_name() << ": Pushing list onto the outputQueue: " << thisQueueName;

        bool successfullyWasSent = false;
        while (!successfullyWasSent && running_flag.load()) {
          TLOG(TLVL_GENERATION) << get_name() << ": Pushing the generated list onto queue " << thisQueueName;

          for (auto const& tp: tps) {
            try {
              outputQueue_->push(tp, queueTimeout_);
              successfullyWasSent = true;
              ++sentCount;
            } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
              std::ostringstream oss_warn;
              oss_warn << "push to output queue \"" << thisQueueName << "\"";
              //ers::warning(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(),
             //                                                   std::chrono::duration_cast<std::chrono::milliseconds>(queueTimeout_).count()));
            }
          }
        }
        
        std::ostringstream oss_prog2;
        oss_prog2 << "Sent hits from file # " << generatedCount;
        //ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog2.str()));
        
        //ERS_LOG(get_name() << " end of while loop");
      }

      std::ostringstream oss_summ;
      oss_summ << ": Exiting the do_work() method, generated " << generatedCount
               << " TP set and successfully sent " << sentCount << " copies. ";
      //ers::info(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TriggerPrimitiveMaker)
