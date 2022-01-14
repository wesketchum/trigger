/**
 * @file TPWriter.hpp TPWriter plugin definition
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_PLUGINS_TPWRITER_HPP_
#define TRIGGER_PLUGINS_TPWRITER_HPP_

#include "trigger/TPSet.hpp"
#include "trigger/tpwriter/Nljs.hpp"

#include <appfwk/DAQModule.hpp>
#include <appfwk/DAQSource.hpp>
#include <appfwk/ThreadHelper.hpp>

#include <fstream>

namespace dunedaq::trigger {

class TPWriter : public dunedaq::appfwk::DAQModule
{
public:
  explicit TPWriter(const std::string& name);

  TPWriter(const TPWriter&) = delete;            ///< TPWriter is not copy-constructible
  TPWriter& operator=(const TPWriter&) = delete; ///< TPWriter is not copy-assignable
  TPWriter(TPWriter&&) = delete;                 ///< TPWriter is not move-constructible
  TPWriter& operator=(TPWriter&&) = delete;      ///< TPWriter is not move-assignable

  void init(const nlohmann::json& obj) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_scrap(const nlohmann::json& obj);

  // Threading
  dunedaq::appfwk::ThreadHelper m_thread;
  void do_work(std::atomic<bool>&);

  // Configuration
  tpwriter::ConfParams m_conf;
  std::unique_ptr<appfwk::DAQSource<TPSet>> m_tpset_source;
  std::ofstream m_output_file;
};

}

#endif // TRIGGER_PLUGINS_TPWRITER_HPP_
