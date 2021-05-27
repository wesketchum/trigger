#include "trigger/TriggerGenericMaker.hpp"

#include "triggeralgs/TriggerActivity.hpp"
#include "triggeralgs/TriggerActivityMaker.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <string>

namespace dunedaq::trigger {

class TriggerActivityMaker 
  : public TriggerGenericMaker < 
    triggeralgs::TriggerPrimitive, 
    triggeralgs::TriggerActivity, 
    triggeralgs::TriggerActivityMaker 
  >
{
public:
  explicit TriggerActivityMaker(const std::string& name) : TriggerGenericMaker(name) { }
  
  TriggerActivityMaker(const TriggerActivityMaker&) = delete;
  TriggerActivityMaker& operator=(const TriggerActivityMaker&) = delete;
  TriggerActivityMaker(TriggerActivityMaker&&) = delete;
  TriggerActivityMaker& operator=(TriggerActivityMaker&&) = delete;

private:
  virtual std::shared_ptr<triggeralgs::TriggerActivityMaker> make_maker(const nlohmann::json& obj);
  
};

} // namespace dunedaq::trigger
