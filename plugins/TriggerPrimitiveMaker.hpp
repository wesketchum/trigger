#include "trigger/TriggerGenericMaker.hpp"

#include "triggeralgs/TriggerPrimitive.hpp"
#include "triggeralgs/TriggerPrimitiveMaker.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <string>

namespace dunedaq::trigger {

class TriggerPrimitiveMaker 
  : public TriggerGenericMaker < 
    triggeralgs::TriggerPrimitive, 
    triggeralgs::TriggerPrimitive
    triggeralgs::TriggerPrimitiveMaker 
  >
{
public:
  explicit TriggerPrimitiveMaker(const std::string& name) : TriggerGenericMaker(name) { }
  
  TriggerPrimitiveMaker(const TriggerPrimitiveMaker&) = delete;
  TriggerPrimitiveMaker& operator=(const TriggerPrimitiveMaker&) = delete;
  TriggerPrimitiveMaker(TriggerPrimitiveMaker&&) = delete;
  TriggerPrimitiveMaker& operator=(TriggerPrimitiveMaker&&) = delete;

private:
  virtual std::shared_ptr<triggeralgs::TriggerPrimitiveMaker> make_maker(const nlohmann::json& obj);
  
};

} // namespace dunedaq::trigger
