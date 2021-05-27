#include "trigger/TriggerGenericMaker.hpp"

#include "triggeralgs/TriggerCandidate.hpp"
#include "triggeralgs/TriggerCandidateMaker.hpp"
#include "triggeralgs/TriggerActivity.hpp"

#include <string>

namespace dunedaq::trigger {

class TriggerCandidateMaker 
  : public TriggerGenericMaker < 
    triggeralgs::TriggerActivity, 
    triggeralgs::TriggerCandidate, 
    triggeralgs::TriggerCandidateMaker 
  >
{
public:
  explicit TriggerCandidateMaker(const std::string& name) : TriggerGenericMaker(name) { }
  
  TriggerCandidateMaker(const TriggerCandidateMaker&) = delete;
  TriggerCandidateMaker& operator=(const TriggerCandidateMaker&) = delete;
  TriggerCandidateMaker(TriggerCandidateMaker&&) = delete;
  TriggerCandidateMaker& operator=(TriggerCandidateMaker&&) = delete;

private:
  virtual std::shared_ptr<triggeralgs::TriggerCandidateMaker> make_maker(const nlohmann::json& obj);
  
};

} // namespace dunedaq::trigger
