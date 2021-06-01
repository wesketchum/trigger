/**
 * @file TriggerPrimitiveMaker.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_PLUGINS_TIMINGTRIGGERPRIMITIVEMAKER_HPP_
#define TRIGGER_PLUGINS_TIMINGTRIGGERPRIMITIVEMAKER_HPP_

#include "trigger/TriggerGenericMaker.hpp"

#include "triggeralgs/TriggerPrimitive.hpp"
#include "triggeralgs/TriggerPrimitiveMaker.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <string>

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include "trigger/Issues.hpp"
#include "trigger/TPSet.hpp"

//#include "trigger/triggerprimitivemaker/Nljs.hpp"
//#include "trigger/triggerprimitivemakerinfo/InfoNljs.hpp"

#include "triggeralgs/TriggerActivity.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace dunedaq {
namespace trigger {
class TriggerPrimitiveMaker : public dunedaq::appfwk::DAQModule
{
    public:
      /**
       * @brief RandomDataListGenerator Constructor
       * @param name Instance name for this RandomDataListGenerator instance
       */
      explicit TriggerPrimitiveMaker(const std::string& name);

      TriggerPrimitiveMaker(const TriggerPrimitiveMaker&) =
        delete; ///< TriggerPrimitiveMaker is not copy-constructible
      TriggerPrimitiveMaker& operator=(const TriggerPrimitiveMaker&) =
        delete; ///< TriggerPrimitiveMaker is not copy-assignable
      TriggerPrimitiveMaker(TriggerPrimitiveMaker&&) =
        delete; ///< TriggerPrimitiveMaker is not move-constructible
      TriggerPrimitiveMaker& operator=(TriggerPrimitiveMaker&&) =
        delete; ///< TriggerPrimitiveMaker is not move-assignable

      void init(const nlohmann::json& obj) override;

    private:
      // Commands
      void do_configure  (const nlohmann::json& obj);
      void do_start      (const nlohmann::json& obj);
      void do_stop       (const nlohmann::json& obj);
      void do_unconfigure(const nlohmann::json& obj);

      // Threading
      dunedaq::appfwk::ThreadHelper thread_;
      void do_work(std::atomic<bool>&);

      // Read csv file
      std::vector<std::vector<int64_t>> ReadCSV(const std::string filename);
      std::string filename;
      std::vector<std::vector<int64_t>> output_vector;

      // Generation
      //ReadCS();
      std::vector<TPSet> GetEvts(std::vector<std::vector<int64_t>> tps_vector);

      // Configuration
      //std::unique_ptr<dunedaq::appfwk::DAQSink<TriggerPrimitive>> outputQueue_;
      using sink_t = dunedaq::appfwk::DAQSink<TPSet>;
      std::unique_ptr<sink_t> outputQueue_;

      std::chrono::milliseconds queueTimeout_;

    };
} // namespace trigger
} // namespace dunedaq

#endif // TRIGGER_PLUGINS_TIMINGTRIGGERPRIMITIVEMAKER_HPP_
