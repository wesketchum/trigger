#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"

#include <string>

namespace dunedaq {
ERS_DECLARE_ISSUE(trigger, InvalidTimeSync, "An invalid TimeSync message was received", ERS_EMPTY)

ERS_DECLARE_ISSUE(trigger, InvalidConfiguration, "An invalid configuration object was received", ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(trigger,
                       SignalTypeError,
                       appfwk::GeneralDAQModuleIssue,
                       "Signal type " << signal_type << " invalid.",
                       ((std::string)name),
                       ((uint32_t)signal_type))

ERS_DECLARE_ISSUE_BASE(trigger,
                       InvalidQueueFatalError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << queueType << " queue was not successfully created.",
                       ((std::string)name),
                       ((std::string)queueType))
} // dunedaq
