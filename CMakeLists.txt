cmake_minimum_required(VERSION 3.12)
project(trigger VERSION 1.0.2)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(ers REQUIRED)
find_package(Boost 1.70.0 COMPONENTS unit_test_framework REQUIRED)
find_package(serialization REQUIRED)
find_package(logging REQUIRED)
find_package(appfwk REQUIRED)
find_package(triggeralgs REQUIRED)
find_package(dfmessages REQUIRED)

##############################################################################
# Main library

# TODO PAR 2021-04-07: triggeralgs doesn't export a
# triggeralgs::triggeralgs target, but instead exports one called
# "SupernovaTrigger". Need to fix that, but go with the flow for now
daq_add_library(TokenManager.cpp TimestampEstimator.cpp TimestampEstimatorBase.cpp TimestampEstimatorSystem.cpp LINK_LIBRARIES appfwk::appfwk logging::logging dfmessages::dfmessages SupernovaTrigger)

##############################################################################
# Codegen

daq_codegen(*.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )

##############################################################################
# Plugins

daq_add_plugin(TimingTriggerCandidateMaker duneDAQModule SCHEMA LINK_LIBRARIES trigger SupernovaTrigger)
daq_add_plugin(ModuleLevelTrigger duneDAQModule LINK_LIBRARIES trigger)
daq_add_plugin(IntervalTriggerCreator duneDAQModule LINK_LIBRARIES trigger TEST)
daq_add_plugin(IntervalTCCreator duneDAQModule LINK_LIBRARIES trigger TEST)
daq_add_plugin(FakeTimeStampedDataGenerator duneDAQModule LINK_LIBRARIES trigger TEST)
daq_add_plugin(FakeDataFlow duneDAQModule LINK_LIBRARIES dfmessages::dfmessages trigger)

##############################################################################
# Integration tests


##############################################################################
# Unit Tests

daq_add_unit_test(TimestampEstimatorSystem_test  LINK_LIBRARIES trigger)
daq_add_unit_test(TimestampEstimator_test        LINK_LIBRARIES trigger)
daq_add_unit_test(TokenManager_test              LINK_LIBRARIES trigger)

##############################################################################

daq_install()
