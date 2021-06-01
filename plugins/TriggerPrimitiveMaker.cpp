#include "TriggerPrimitiveMaker.hpp"

#include "trigger/AlgorithmPlugins.hpp"
#include "trigger/triggerprimitivemaker/Nljs.hpp"

namespace dunedaq::trigger {

  std::shared_ptr<triggeralgs::TriggerPrimitiveMaker>
  TriggerPrimitiveMaker::make_maker(const nlohmann::json& obj)
  {
    auto params = obj.get<triggerprimitivemaker::Conf>();
    return make_tp_maker(params.primitive_maker);
  }

} // namespace dunedaq::trigger

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TriggerPrimitiveMaker)
