#include "trigger/FragmentMakers.hpp"
#include "dataformats/trigger/TriggerPrimitivesFragment.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

namespace dunedaq::trigger {

//======================================================================

dataformats::FragmentHeader
create_fragment_header(const dfmessages::DataRequest& dr)
{
  dataformats::FragmentHeader fh;
  fh.size = sizeof(fh);
  fh.trigger_number = dr.trigger_number;
  fh.trigger_timestamp = dr.trigger_timestamp;
  fh.window_begin = dr.window_begin;
  fh.window_end = dr.window_end;
  fh.run_number = dr.run_number;
  // fh.element_id = { m_geoid.system_type, m_geoid.region_id, m_geoid.element_id };
  // fh.fragment_type = static_cast<dataformats::fragment_type_t>(ReadoutType::fragment_type);
  return fh;
}

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
    dataformats::TriggerPrimitivesFragment::TriggerPrimitive& tp_fragment = tpf->primitives[counter++];
    tp_fragment.time_start = tp.time_start;
    tp_fragment.time_peak = tp.time_peak;
    tp_fragment.time_over_threshold = tp.time_over_threshold;
    tp_fragment.channel = tp.channel;
    tp_fragment.adc_integral = tp.adc_integral;
    tp_fragment.adc_peak = tp.adc_peak;
    tp_fragment.detid = tp.detid;
    tp_fragment.type = static_cast<uint32_t>(tp.type);
    tp_fragment.algorithm = static_cast<uint32_t>(tp.algorithm);
    tp_fragment.version = tp.version;
    tp_fragment.flag = tp.flag;
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
    const dataformats::TriggerPrimitivesFragment::TriggerPrimitive& tp_fragment = tpf->primitives[i];

    triggeralgs::TriggerPrimitive tp;
    tp.time_start = tp_fragment.time_start;
    tp.time_peak = tp_fragment.time_peak;
    tp.time_over_threshold = tp_fragment.time_over_threshold;
    tp.channel = tp_fragment.channel;
    tp.adc_integral = tp_fragment.adc_integral;
    tp.adc_peak = tp_fragment.adc_peak;
    tp.detid = tp_fragment.detid;
    tp.type = static_cast<triggeralgs::TriggerPrimitive::Type>(tp_fragment.type);
    tp.algorithm = static_cast<triggeralgs::TriggerPrimitive::Algorithm>(tp_fragment.algorithm);
    tp.version = tp_fragment.version;
    tp.flag = tp_fragment.flag;

    tps.push_back(tp);
  }

  return tps;
}

} // namespace dunedaq
