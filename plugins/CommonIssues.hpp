#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"

#include <string>

namespace dunedaq {

ERS_DECLARE_ISSUE_BASE(trigger,
                       ConfigurationError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << message << " queue was not successfully configured.",
                       ((std::string)name),
                       ((std::string)message))

ERS_DECLARE_ISSUE_BASE(trigger,
                       InvalidQueueFatalError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << queueType << " queue was not successfully created.",
                       ((std::string)name),
                       ((std::string)queueType))

} // namespace dunedaq
