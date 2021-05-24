#ifndef TRIGGER_INCLUDE_TRIGGER_ALGORITHMPLUGINS_HPP_
#define TRIGGER_INCLUDE_TRIGGER_ALGORITHMPLUGINS_HPP_

#include "triggeralgs/TriggerActivityMaker.hpp"
#include "triggeralgs/TriggerCandidateMaker.hpp"
#include "triggeralgs/TriggerDecisionMaker.hpp"

#include "cetlib/BasicPluginFactory.h"

#include <memory>
#include <string>

/**
 * @brief Declare the function that will be called by the plugin loader
 * @param klass Class to be defined as a DUNE TAMaker module
 */
#define DEFINE_DUNE_TA_MAKER(klass)                                                                                    \
  extern "C"                                                                                                           \
  {                                                                                                                    \
    std::shared_ptr<triggeralgs::TriggerActivityMaker> make()                                                          \
    {                                                                                                                  \
      return std::shared_ptr<triggeralgs::TriggerActivityMaker>(new klass);                                            \
    }                                                                                                                  \
  }

namespace dunedaq::trigger {
/**
 * @brief Load a TriggerActivityMaker plugin and return a shared_ptr to the contained
 * class
 * @param plugin_name Name of the plugin
 * @return shared_ptr to created TriggerActivityMaker instance
 */
inline std::shared_ptr<triggeralgs::TriggerActivityMaker>
make_ta_maker(std::string const& plugin_name)
{
  static cet::BasicPluginFactory bpf("duneTAMaker", "make");

  // TODO PAR 2021-04-06: Rethrow any cetlib exception as an ERS issue
  return bpf.makePlugin<std::shared_ptr<triggeralgs::TriggerActivityMaker>>(plugin_name);
}

}

/**
 * @brief Declare the function that will be called by the plugin loader
 * @param klass Class to be defined as a DUNE TCMaker module
 */
#define DEFINE_DUNE_TC_MAKER(klass)                                                                                    \
  extern "C"                                                                                                           \
  {                                                                                                                    \
    std::shared_ptr<triggeralgs::TriggerCandidateMaker> make()                                                         \
    {                                                                                                                  \
      return std::shared_ptr<triggeralgs::TriggerCandidateMaker>(new klass);                                           \
    }                                                                                                                  \
  }

namespace dunedaq::trigger {
/**
 * @brief Load a TriggerCandidateMaker plugin and return a shared_ptr to the contained
 * class
 * @param plugin_name Name of the plugin
 * @return shared_ptr to created TriggerCandidateMaker instance
 */
inline std::shared_ptr<triggeralgs::TriggerCandidateMaker>
make_tc_maker(std::string const& plugin_name)
{
  static cet::BasicPluginFactory bpf("duneTCMaker", "make");

  // TODO PAR 2021-04-06: Rethrow any cetlib exception as an ERS issue
  return bpf.makePlugin<std::shared_ptr<triggeralgs::TriggerCandidateMaker>>(plugin_name);
}

}

/**
 * @brief Declare the function that will be called by the plugin loader
 * @param klass Class to be defined as a DUNE TDMaker module
 */
#define DEFINE_DUNE_TD_MAKER(klass)                                                                                    \
  extern "C"                                                                                                           \
  {                                                                                                                    \
    std::shared_ptr<triggeralgs::TriggerDecisionMaker> make()                                                          \
    {                                                                                                                  \
      return std::shared_ptr<triggeralgs::TriggerDecisionMaker>(new klass);                                            \
    }                                                                                                                  \
  }

namespace dunedaq::trigger {
/**
 * @brief Load a TriggerDecisionMaker plugin and return a shared_ptr to the contained
 * class
 * @param plugin_name Name of the plugin
 * @return shared_ptr to created TriggerDecisionMaker instance
 */
inline std::shared_ptr<triggeralgs::TriggerDecisionMaker>
make_td_maker(std::string const& plugin_name)
{
  static cet::BasicPluginFactory bpf("duneTDMaker", "make");

  // TODO PAR 2021-04-06: Rethrow any cetlib exception as an ERS issue
  return bpf.makePlugin<std::shared_ptr<triggeralgs::TriggerDecisionMaker>>(plugin_name);
}

}

#endif // TRIGGER_INCLUDE_TRIGGER_ALGORITHMPLUGINS_HPP_
