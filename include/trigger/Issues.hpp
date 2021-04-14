#include "ers/Issue.hpp"

namespace dunedaq {
ERS_DECLARE_ISSUE(trigger, InvalidTimeSync, "An invalid TimeSync message was received", ERS_EMPTY)

ERS_DECLARE_ISSUE(trigger, InvalidConfiguration, "An invalid configuration object was received", ERS_EMPTY)
} // dunedaq
