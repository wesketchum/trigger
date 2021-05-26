#include "trigger/TASet.hpp"
#include "serialization/Serialization.hpp"

#include <chrono>
#include <iostream>
#include <random>

int main()
{
  dunedaq::trigger::TASet taset;
  std::vector<uint8_t> bytes = dunedaq::serialization::serialize(taset, dunedaq::serialization::kMsgPack); // NOLINT(build/unsigned)
  dunedaq::trigger::TASet set_recv = dunedaq::serialization::deserialize<dunedaq::trigger::TASet>(bytes);
}
