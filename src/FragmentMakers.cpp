#include "trigger/FragmentMakers.hpp"
#include "dataformats/trigger/TriggerPrimitivesFragment.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

namespace dunedaq::trigger {

//======================================================================
std::unique_ptr<dataformats::Fragment>
make_fragment(std::vector<TPSet>& tpsets)
{
  std::vector<triggeralgs::TriggerPrimitive> tps;
  for (auto const& tpset : tpsets) {
    for (auto const& tp : tpset.objects) {
      tps.push_back(tp);
    }
  }

  return make_fragment(tps);
}

//======================================================================
std::unique_ptr<dataformats::Fragment>
make_fragment(std::vector<triggeralgs::TriggerPrimitive>& tps)
{
  size_t n_bytes = sizeof(dataformats::TriggerPrimitivesFragment) +
                   tps.size() * sizeof(dataformats::TriggerPrimitivesFragment::TriggerPrimitive);
  std::unique_ptr<uint8_t[]> buffer(new uint8_t[n_bytes]);
  dataformats::TriggerPrimitivesFragment* tpf = reinterpret_cast<dataformats::TriggerPrimitivesFragment*>(buffer.get());
  tpf->version = dataformats::TriggerPrimitivesFragment::s_tpf_version;
  tpf->n_trigger_primitives = tps.size();
  size_t counter = 0;
  for (auto const& tp : tps) {
    dataformats::TriggerPrimitivesFragment::TriggerPrimitive& fragment_tp = tpf->at(counter++);
    fragment_tp.time_start = tp.time_start;
    fragment_tp.time_peak = tp.time_peak;
    fragment_tp.time_over_threshold = tp.time_over_threshold;
    fragment_tp.channel = tp.channel;
    fragment_tp.adc_integral = tp.adc_integral;
    fragment_tp.adc_peak = tp.adc_peak;
    fragment_tp.detid = tp.detid;
    fragment_tp.type = static_cast<uint32_t>(tp.type);
    fragment_tp.algorithm = static_cast<uint32_t>(tp.algorithm);
    fragment_tp.version = tp.version;
    fragment_tp.flag = tp.flag;
  }

  std::unique_ptr<dataformats::Fragment> frag = std::make_unique<dataformats::Fragment>(buffer.get(), n_bytes);
  return frag;
}

//======================================================================
std::vector<triggeralgs::TriggerPrimitive>
read_fragment_to_trigger_primitives(dataformats::Fragment* frag)
{
  std::vector<triggeralgs::TriggerPrimitive> tps;
  const dataformats::TriggerPrimitivesFragment* tpf =
    reinterpret_cast<const dataformats::TriggerPrimitivesFragment*>(frag->get_data());
  for (uint64_t i = 0; i < tpf->n_trigger_primitives; ++i) {
    const dataformats::TriggerPrimitivesFragment::TriggerPrimitive& fragment_tp = tpf->at(i);

    triggeralgs::TriggerPrimitive tp;
    tp.time_start = fragment_tp.time_start;
    tp.time_peak = fragment_tp.time_peak;
    tp.time_over_threshold = fragment_tp.time_over_threshold;
    tp.channel = fragment_tp.channel;
    tp.adc_integral = fragment_tp.adc_integral;
    tp.adc_peak = fragment_tp.adc_peak;
    tp.detid = fragment_tp.detid;
    tp.type = static_cast<triggeralgs::TriggerPrimitive::Type>(fragment_tp.type);
    tp.algorithm = static_cast<triggeralgs::TriggerPrimitive::Algorithm>(fragment_tp.algorithm);
    tp.version = fragment_tp.version;
    tp.flag = fragment_tp.flag;

    tps.push_back(tp);
  }

  return tps;
}

} // namespace dunedaq
