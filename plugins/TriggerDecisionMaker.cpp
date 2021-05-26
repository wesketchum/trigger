#include "TriggerDecisionMaker.hpp"

#include "trigger/AlgorithmPlugins.hpp"
#include "trigger/triggerdecisionmaker/Nljs.hpp"

namespace dunedaq::trigger {

  std::shared_ptr<triggeralgs::TriggerDecisionMaker>
  TriggerDecisionMaker::make_maker(const nlohmann::json& obj)
  {
    auto params = obj.get<triggerdecisionmaker::Conf>();
    return make_td_maker(params.decision_maker);
  }

} // namespace dunedaq::trigger

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TriggerDecisionMaker)
