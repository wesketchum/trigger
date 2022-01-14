/**
 * @file TPWriter.cpp TPWriter plugin implementation
 * 
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TPWriter.hpp"
#include "trigger/Issues.hpp"
#include <appfwk/DAQModuleHelper.hpp>
#include <appfwk/Queue.hpp>

namespace dunedaq::trigger {

TPWriter::TPWriter(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_thread(std::bind(&TPWriter::do_work, this, std::placeholders::_1))
  , m_tpset_source()
{
  register_command("conf",  &TPWriter::do_configure);
  register_command("start", &TPWriter::do_start);
  register_command("stop",  &TPWriter::do_stop);
  register_command("scrap", &TPWriter::do_scrap);
}

void
TPWriter::init(const nlohmann::json& obj)
{
  m_tpset_source.reset(new appfwk::DAQSource<TPSet>(appfwk::queue_inst(obj, "input")));
}

void
TPWriter::do_configure(const nlohmann::json& obj)
{
  m_conf = obj.get<tpwriter::ConfParams>();

  m_output_file.open(m_conf.filename);
  if (!m_output_file || m_output_file.bad()) {
    throw BadTPInputFile(ERS_HERE, get_name(), m_conf.filename);
  }
}

void
TPWriter::do_start(const nlohmann::json& /*args*/)
{
  m_thread.start_working_thread("tpmaker");
}

void
TPWriter::do_stop(const nlohmann::json& /*args*/)
{
  m_thread.stop_working_thread();
}

void
TPWriter::do_scrap(const nlohmann::json& /*args*/)
{
}

void
TPWriter::do_work(std::atomic<bool>& running_flag)
{
  while (running_flag.load()) {
    TPSet tpset;
    try {
      m_tpset_source->pop(tpset, std::chrono::milliseconds(100));
    } catch(appfwk::QueueTimeoutExpired& e) {
      continue;
    }
    for(auto const& tp: tpset.objects) {
      m_output_file << "\t" << tp.time_start << "\t" << tp.time_over_threshold << "\t" << tp.time_peak << "\t" << tp.channel << "\t" << tp.adc_integral << "\t" <<
        tp.adc_peak << "\t" << tp.detid << "\t" << tp.type << "\t" << std::endl;
    }
  } // end while(running_flag.load())
}

} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigger::TPWriter)
