cmake_minimum_required(VERSION 3.12)
project(trigger VERSION 0.0.1)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(ers REQUIRED)
find_package(Boost 1.70.0 COMPONENTS unit_test_framework REQUIRED)
find_package(serialization REQUIRED)
find_package(logging REQUIRED)
find_package(appfwk REQUIRED)
find_package(triggeralgs REQUIRED)

##############################################################################
# Main library

daq_add_library( LINK_LIBRARIES appfwk::appfwk logging::logging)

##############################################################################
# Plugins

# daq_add_plugin(... ... LINK_LIBRARIES trigger)

##############################################################################
# Integration tests



##############################################################################
# Unit Tests

# daq_add_unit_test(ComponentRequest_test        LINK_LIBRARIES trigger)

##############################################################################

daq_install()
