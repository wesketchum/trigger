#include "trigger/TriggerGenericMaker.hpp"

#include "triggeralgs/TriggerDecision.hpp"
#include "triggeralgs/TriggerDecisionMaker.hpp"
#include "triggeralgs/TriggerCandidate.hpp"

#include <string>

namespace dunedaq::trigger {

class TriggerDecisionMaker 
  : public TriggerGenericMaker < 
    triggeralgs::TriggerCandidate, 
    triggeralgs::TriggerDecision, 
    triggeralgs::TriggerDecisionMaker 
  >
{
public:
  explicit TriggerDecisionMaker(const std::string& name) : TriggerGenericMaker(name) { }
  
  TriggerDecisionMaker(const TriggerDecisionMaker&) = delete;
  TriggerDecisionMaker& operator=(const TriggerDecisionMaker&) = delete;
  TriggerDecisionMaker(TriggerDecisionMaker&&) = delete;
  TriggerDecisionMaker& operator=(TriggerDecisionMaker&&) = delete;

private:
  virtual std::shared_ptr<triggeralgs::TriggerDecisionMaker> make_maker(const nlohmann::json& obj);
  
};

} // namespace dunedaq::trigger
