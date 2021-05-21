/**
 * @file Issues.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_INCLUDE_TRIGGER_ISSUES_HPP_
#define TRIGGER_INCLUDE_TRIGGER_ISSUES_HPP_

#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"

#include <string>

// NOLINTNEXTLINE(build/define_used)
#define TLVL_ENTER_EXIT_METHODS 10
// NOLINTNEXTLINE(build/define_used)
#define TLVL_GENERATION 11
// NOLINTNEXTLINE(build/define_used)
#define TLVL_CANDIDATE 15

namespace dunedaq {

ERS_DECLARE_ISSUE(trigger, InvalidConfiguration, "An invalid configuration object was received", ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(dunetrigger,
                       ProgressUpdate,
                       appfwk::GeneralDAQModuleIssue,
                       message,
                       ((std::string)name),
                       ((std::string)message))

ERS_DECLARE_ISSUE_BASE(dunetrigger,
                       InvalidQueueFatalError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << queueType << " queue was not successfully created.",
                       ((std::string)name),
                       ((std::string)queueType))

ERS_DECLARE_ISSUE_BASE(trigger,
                       SignalTypeError,
                       appfwk::GeneralDAQModuleIssue,
                       "Signal type " << signal_type << " invalid.",
                       ((std::string)name),
                       ((uint32_t)signal_type)) // NOLINT(build/unsigned)

ERS_DECLARE_ISSUE_BASE(trigger,
                       InvalidQueueFatalError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << queueType << " queue was not successfully created.",
                       ((std::string)name),
                       ((std::string)queueType))

} // namespace dunedaq

#endif // TRIGGER_INCLUDE_TRIGGER_ISSUES_HPP_
