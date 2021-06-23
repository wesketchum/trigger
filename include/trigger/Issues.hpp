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
                       
ERS_DECLARE_ISSUE_BASE(trigger,
                       AlgorithmFatalError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << algorithm << " failed to run.",
                       ((std::string)name),
                       ((std::string)algorithm))
                       
ERS_DECLARE_ISSUE_BASE(trigger,
                       UnknownSetError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << algorithm << " encountered an unknown Set type.",
                       ((std::string)name),
                       ((std::string)algorithm))
                       
ERS_DECLARE_ISSUE_BASE(trigger,
                       InconsistentSetTimeError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << algorithm << " maker encountered Sets with inconsistent start/end times.",
                       ((std::string)name),
                       ((std::string)algorithm))

ERS_DECLARE_ISSUE_BASE(trigger,
                       BadTPInputFile,
                       appfwk::GeneralDAQModuleIssue,
                       "Problem opening file " << filename,
                       ((std::string)name),
                       ((std::string)filename))

ERS_DECLARE_ISSUE_BASE(trigger,
                       UnsortedTP,
                       appfwk::GeneralDAQModuleIssue,
                       "TP with time_start " << time_start << "is higher than time_start of last TP and will be ignored." ,
                       ((std::string)name),
                       ((int64_t)time_start))
} // namespace dunedaq

#endif // TRIGGER_INCLUDE_TRIGGER_ISSUES_HPP_
