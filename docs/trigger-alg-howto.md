# How to write a trigger algorithm

The DUNE DAQ data selection software separates physics algorithms
(which make trigger activity and trigger candidate objects) from
concerns about how the data objects are packaged and transported
through the data selection system.

Physics algorithms are implemented in the [https://github.com/DUNE-DAQ/triggeralgs](`triggeralgs`) package. To create a new physics algorithm, create a class that derives from https://github.com/DUNE-DAQ/triggeralgs/blob/develop/include/triggeralgs/TriggerActivityMaker.hpp or https://github.com/DUNE-DAQ/triggeralgs/blob/develop/include/triggeralgs/TriggerCandidateMaker.hpp as appropriate