#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include "logging/Logging.hpp"

#include "triggeralgs/TriggerActivity.hpp"
#include "triggeralgs/TriggerActivityMaker.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <memory>
#include <string>

namespace dunedaq::trigger {
class TriggerActivityMaker : public dunedaq::appfwk::DAQModule
{
public:
  explicit TriggerActivityMaker(const std::string& name);

  TriggerActivityMaker(const TriggerActivityMaker&) = delete;
  TriggerActivityMaker& operator=(const TriggerActivityMaker&) = delete;
  TriggerActivityMaker(TriggerActivityMaker&&) = delete;
  TriggerActivityMaker& operator=(TriggerActivityMaker&&) = delete;

  void init(const nlohmann::json& obj) override;

private:
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_configure(const nlohmann::json& obj);

  dunedaq::appfwk::ThreadHelper m_thread;
  void do_work(std::atomic<bool>&);

  using source_t = dunedaq::appfwk::DAQSource<triggeralgs::TriggerPrimitive>;
  std::unique_ptr<source_t> m_input_queue;

  using sink_t = dunedaq::appfwk::DAQSink<triggeralgs::TriggerActivity>;
  std::unique_ptr<sink_t> m_output_queue;

  std::chrono::milliseconds m_queue_timeout;

  std::shared_ptr<triggeralgs::TriggerActivityMaker> m_activity_maker;
};

} // namespace dunedaq::trigger
