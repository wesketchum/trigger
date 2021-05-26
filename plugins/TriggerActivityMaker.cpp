#include "TriggerActivityMaker.hpp"

#include "trigger/AlgorithmPlugins.hpp"
#include "trigger/triggeractivitymaker/Nljs.hpp"

namespace dunedaq::trigger {

  std::shared_ptr<triggeralgs::TriggerActivityMaker>
  TriggerActivityMaker::make_maker(const nlohmann::json& obj)
  {
    auto params = obj.get<triggeractivitymaker::Conf>();
    return make_ta_maker(params.activity_maker);
  }

} // namespace dunedaq::trigger

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TriggerActivityMaker)
