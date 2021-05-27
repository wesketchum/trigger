/**
 * @file TriggerCandidateMaker.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
 
#include "TriggerCandidateMaker.hpp"

#include "trigger/AlgorithmPlugins.hpp"
#include "trigger/triggercandidatemaker/Nljs.hpp"

#include <memory>

namespace dunedaq::trigger {

  std::shared_ptr<triggeralgs::TriggerCandidateMaker>
  TriggerCandidateMaker::make_maker(const nlohmann::json& obj)
  {
    auto params = obj.get<triggercandidatemaker::Conf>();
    return make_tc_maker(params.candidate_maker);
  }

} // namespace dunedaq::trigger

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TriggerCandidateMaker)
