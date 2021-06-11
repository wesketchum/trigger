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
      m_tpset_sink(),
      queueTimeout_(100)
      {
      register_command("conf"       , &TriggerPrimitiveMaker::do_configure  );
      register_command("start"      , &TriggerPrimitiveMaker::do_start      );
      register_command("stop"       , &TriggerPrimitiveMaker::do_stop       );
      register_command("scrap"      , &TriggerPrimitiveMaker::do_unconfigure);
    }

    void TriggerPrimitiveMaker::init(const nlohmann::json& obj) {
        m_tpset_sink.reset(
            new appfwk::DAQSink<TPSet>(appfwk::queue_inst(obj, "tpset_sink")));
    }

    void
    TriggerPrimitiveMaker::do_configure(const nlohmann::json& obj)
    {
       m_conf = obj.get<triggerprimitivemaker::ConfParams>();
       m_number_of_loops = m_conf.number_of_loops;
       m_chunk_offset = m_conf.chunk_offset;
       m_chunk_width = m_conf.chunk_width;
       std::ifstream file(m_conf.filename);
       m_number_of_rows = 0;
       std::vector<TriggerPrimitive> all_tps; 
       while(file){
         TriggerPrimitive tp;
         file >> tp.time_start >> tp.time_over_threshold >> tp.time_peak >> tp.channel >> tp.adc_integral >> tp.adc_peak >> tp.detid >> tp.type;
         all_tps.push_back(tp);
         m_tpsets[m_number_of_rows].objects.push_back(tp);
         m_number_of_rows++;
       }
       file.close();

       m_number_of_chunks = (m_number_of_rows-m_chunk_offset)/m_chunk_width;
       uint64_t currentChunk = 0;
       while(currentChunk < m_number_of_chunks) {
         TPSet currentTPSet;
         uint64_t start_index = m_chunk_offset + currentChunk*m_chunk_width;
         currentTPSet.start_time = all_tps[start_index].time_start;
         currentTPSet.end_time = all_tps[start_index].time_peak;
         for(uint64_t index = start_index; index < m_chunk_offset + (currentChunk+1)*m_chunk_width; index++){
           currentTPSet.objects.push_back(all_tps[index]);
           if ((uint64_t)all_tps[index].time_start < currentTPSet.start_time) {
             currentTPSet.start_time = (uint64_t)all_tps[index].time_start;
	   }
           if ((uint64_t)all_tps[index].time_peak > currentTPSet.end_time) {
	     currentTPSet.end_time = (uint64_t)all_tps[index].time_peak;
	   }
           currentTPSet.objects.push_back(all_tps[index]);
          }
         m_tpsets.push_back(currentTPSet);
         currentChunk++;
       }
          
    }

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

    void TriggerPrimitiveMaker::do_work(std::atomic<bool>& running_flag) {
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
      size_t generatedCount = 0;
      size_t sentCount = 0;
      uint64_t currentIteration = 0;
      uint64_t currentChunk = 0;
      while (running_flag.load() && currentIteration < m_number_of_loops) {
        uint64_t start_time_old = 0;
        uint64_t time_difference = 0;
        while (currentChunk < m_number_of_chunks) {
          if (currentChunk > 0) {
	    time_difference = m_tpsets[currentChunk].start_time - start_time_old;
            //wait_until() // wait until time_difference
          }
          start_time_old = m_tpsets[currentChunk].start_time;

          TLOG(TLVL_GENERATION) << get_name() << ": Start of sleep between sends";
          std::this_thread::sleep_for(std::chrono::nanoseconds(1000000000));

          if (m_number_of_rows == 0) {
            std::ostringstream oss_prog;
            oss_prog << "TPs packet has size 0, continuing!";
            //ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));
            continue; 
          } else {
            std::ostringstream oss_prog;
            oss_prog << "Generated TPs #" << generatedCount << " last TPs packet has size " << m_number_of_rows;
            //ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));
          }
  
          generatedCount+=m_number_of_rows;
          
          std::string thisQueueName = m_tpset_sink->get_name();
          TLOG(TLVL_GENERATION) << get_name() << ": Pushing list onto the outputQueue: " << thisQueueName;

          bool successfullyWasSent = false;
          while (!successfullyWasSent && running_flag.load()) {
            TLOG(TLVL_GENERATION) << get_name() << ": Pushing the generated list onto queue " << thisQueueName;

            try {
              m_tpset_sink->push(m_tpsets[currentChunk], queueTimeout_);
              successfullyWasSent = true;
              ++sentCount;
            } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
              std::ostringstream oss_warn;
              oss_warn << "push to output queue \"" << thisQueueName << "\"";
              //ers::warning(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(),
              //                                                   std::chrono::duration_cast<std::chrono::milliseconds>(queueTimeout_).count()));
            }
          }
          std::ostringstream oss_prog2;
          oss_prog2 << "Sent hits from file # " << generatedCount;
          //ers::debug(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_prog2.str()));
        
          //ERS_LOG(get_name() << " end of while loop");
          currentChunk++;
        }
        currentIteration++;
      }	
      std::ostringstream oss_summ;
      oss_summ << ": Exiting the do_work() method, generated " << generatedCount
               << " TP set and successfully sent " << sentCount << " copies. ";
      //ers::info(dunedaq::dunetrigger::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
      TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
     }
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TriggerPrimitiveMaker)
