/**
 * @file TriggerGenericMaker.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
 
#ifndef TRIGGER_SRC_TRIGGER_TRIGGERGENERICMAKER_HPP_
#define TRIGGER_SRC_TRIGGER_TRIGGERGENERICMAKER_HPP_

#include "trigger/Set.hpp"
#include "trigger/Issues.hpp"
 
#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "logging/Logging.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq::trigger {

// Forward declare the class encapsulating partial specifications of do_work
template<class IN, class OUT, class MAKER>
class TriggerGenericWorker;

template<class IN, class OUT, class MAKER>
class TriggerGenericMaker : public dunedaq::appfwk::DAQModule
{
friend class TriggerGenericWorker<IN,OUT,MAKER>;
  
public:

  explicit TriggerGenericMaker(const std::string& name)
    : DAQModule(name)
    , worker(*this)
    , m_thread(std::bind(&TriggerGenericMaker::do_work, this, std::placeholders::_1))
    , m_input_queue(nullptr)
    , m_output_queue(nullptr)
    , m_queue_timeout(100)
    , m_algorithm_name("[uninitialized]")
  {
    register_command("start", &TriggerGenericMaker::do_start);
    register_command("stop", &TriggerGenericMaker::do_stop);
    register_command("conf", &TriggerGenericMaker::do_configure);
  }
  
  virtual ~TriggerGenericMaker() { }

  TriggerGenericMaker(const TriggerGenericMaker&) = delete;
  TriggerGenericMaker& operator=(const TriggerGenericMaker&) = delete;
  TriggerGenericMaker(TriggerGenericMaker&&) = delete;
  TriggerGenericMaker& operator=(TriggerGenericMaker&&) = delete;

  void init(const nlohmann::json& obj) override
  {
    m_input_queue.reset(new source_t(appfwk::queue_inst(obj, "input")));
    m_output_queue.reset(new sink_t(appfwk::queue_inst(obj, "output")));
  }

protected:

  void set_algorithm_name(const std::string &name) 
  { 
    m_algorithm_name = name;
  }

private:

  TriggerGenericWorker<IN,OUT,MAKER> worker;

  dunedaq::appfwk::ThreadHelper m_thread;
  
  size_t m_received_count;
  size_t m_sent_count;
  
  using source_t = dunedaq::appfwk::DAQSource<IN>;
  std::unique_ptr<source_t> m_input_queue;

  using sink_t = dunedaq::appfwk::DAQSink<OUT>;
  std::unique_ptr<sink_t> m_output_queue;

  std::chrono::milliseconds m_queue_timeout;
  
  std::string m_algorithm_name;

  std::shared_ptr<MAKER> m_maker;
  
  // This should return a shared_ptr to the MAKER created from conf command
  // arguments, and also call set_algorithm_name.
  virtual std::shared_ptr<MAKER> make_maker(const nlohmann::json& obj) = 0;
  
  void do_start(const nlohmann::json& /*obj*/)
  {
    m_received_count = 0;
    m_sent_count = 0;
    m_thread.start_working_thread();
  }
  
  void do_stop(const nlohmann::json& /*obj*/)
  {
    m_thread.stop_working_thread();
  }
  
  void do_configure(const nlohmann::json& obj)
  {
    m_maker = make_maker(obj);
  }
  
  void do_work(std::atomic<bool> &running_flag)
  {
    worker.do_work(running_flag);
  }
  
  bool receive(IN &in) {
    try {
      m_input_queue->pop(in, m_queue_timeout);
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      // it is perfectly reasonable that there might be no data in the queue
      // some fraction of the times that we check, so we just continue on and try again
      return false;
    }
    ++m_received_count;
    return true;
  }
  
  bool send(const OUT &out) {
    try {
      m_output_queue->push(out, m_queue_timeout);
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      ers::warning(excpt);
      return false;
    }
    ++m_sent_count;
    return true;
  }
  
};

// To handle the different unpacking schemes implied by different templates,
// do_work is broken out into its own template class that is a friend of 
// TriggerGenericMaker. C++ still does not support partial specification of a 
// single method in a template class, so this approach is the least redundant
// way to achieve that functionality

// The base template assumes the MAKER has an operator() with the signature
// operator()(IN, std::vector<OUT>)
template<class IN, class OUT, class MAKER>
class TriggerGenericWorker
{
public:
  explicit TriggerGenericWorker(TriggerGenericMaker<IN, OUT, MAKER> &parent) : m_parent(parent) { }
  
  TriggerGenericMaker<IN, OUT, MAKER> &m_parent;
  
  void do_work(std::atomic<bool> &running_flag)
  {
    while (running_flag.load()) {
      IN in;
      if (!m_parent.receive(in)) {
        continue; //nothing to do
      }
  
      std::vector<OUT> out_vec; // 1 input -> many outputs
      try {
        m_parent.m_maker->operator()(in, out_vec);
      } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
        ers::error(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
        continue;
      }

      while (out_vec.size() && running_flag.load()) {
        if (m_parent.send(out_vec.back())) {
          out_vec.pop_back();
        }
      }
    } // end while (running_flag.load())

    TLOG() << ": Exiting do_work() method, received " << m_parent.m_received_count 
           << " inputs and successfully sent " << m_parent.m_sent_count << " outputs. ";
  }
};

// Partial specilization for IN = Set<A>, OUT = Set<B> and assumes the MAKER has:
// operator()(A, std::vector<B>)
template<class A, class B, class MAKER> 
class TriggerGenericWorker<Set<A>, Set<B>, MAKER> 
{
public:
  explicit TriggerGenericWorker(TriggerGenericMaker<Set<A>, Set<B>, MAKER> &parent) : m_parent(parent) { }
  
  TriggerGenericMaker<Set<A>, Set<B>, MAKER> &m_parent;
  
  void do_work(std::atomic<bool> &running_flag)
  {
    while (running_flag.load()) {
      Set<A> in;
      if (!m_parent.receive(in)) {
        continue; //nothing to do
      }

      Set<B> out; // 1 input set -> 1 output set
      switch (in.type) {
        case Set<A>::Type::kPayload:
          // call operator for each of the objects in the set
          for (size_t i = 0; i < in.objects.size(); i++) {
            std::vector<B> out_vec;
            try {
              m_parent.m_maker->operator()(in.objects[i], out_vec);
            } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
              ers::error(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
              continue;
            }
            out.objects.insert(out.objects.end(), out_vec.begin(), out_vec.end());
          }
          out.type = Set<B>::Type::kPayload;
          break;
        case Set<A>::Type::kHeartbeat:
          // forward the heartbeat
          { 
            Set<B> heartbeat;
            heartbeat.seqno = m_parent.m_sent_count;
            heartbeat.type = Set<B>::Type::kHeartbeat;
            heartbeat.start_time = in.start_time;
            heartbeat.end_time = in.end_time;
            while (running_flag.load()) {
              if (m_parent.send(heartbeat)) {
                break;
              }
            }
          }
          // flush the maker
          try {
            m_parent.m_maker->flush(in.end_time, out.objects);
          } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
            ers::error(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
            continue;
          }
          out.type = Set<B>::Type::kPayload;
          break;
        case Set<A>::Type::kUnknown:
          ers::error(UnknownSetError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
          break;
      }
          
      if (out.objects.size() > 0) {
        out.seqno = m_parent.m_sent_count;
        
        // TODO BJL May-27-2021 Set<T> should implement some helper methods to 
        // populate these fields. in lieu of that, this is an approximation
        out.from_detids.resize(out.objects.size());
        for (size_t i = 0; i < out.objects.size(); i++) {
          out.from_detids[i] = out.objects[i].detid;
        }
        out.start_time = out.objects[0].time_start;
        // TODO BJL May-27-2021 TP and TA both have time_start, but only TA has time_end
        // really, Set<T> should decide what is desired here, not the maker
        // for now, let end_time be the largest time_start (start_time the smallest)
        out.end_time = out.objects[out.objects.size()-1].time_start;
        
        while (running_flag.load()) {
          if (m_parent.send(out)) {
            break;
          }
        }
      }
      
    } // end while (running_flag.load())

    TLOG() << ": Exiting do_work() method, received " << m_parent.m_received_count 
           << " inputs and successfully sent " << m_parent.m_sent_count << " outputs. ";
  }
};

// Partial specilization for IN = Set<A> and assumes the the MAKER has:
// operator()(A, std::vector<OUT>)
template<class A, class OUT, class MAKER> 
class TriggerGenericWorker<Set<A>, OUT, MAKER> 
{
public:
  explicit TriggerGenericWorker(TriggerGenericMaker<Set<A>, OUT, MAKER> &parent) : m_parent(parent) { }
  
  TriggerGenericMaker<Set<A>, OUT, MAKER> &m_parent;
  
  void do_work(std::atomic<bool> &running_flag)
  {
    while (running_flag.load()) {
      Set<A> in;
      if (!m_parent.receive(in)) {
        continue; //nothing to do
      }

      std::vector<OUT> out_vec; // 1 input set -> many outputs
      switch (in.type) {
        case Set<A>::Type::kPayload:
          for (size_t i = 0; i < in.objects.size(); i++) {
            try {
              m_parent.m_maker->operator()(in.objects[i], out_vec);
            } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
              ers::error(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
            }
          }
          break;
        case Set<A>::Type::kHeartbeat:
          // TODO BJL May-28-2021 should anything happen with the heartbeat when OUT is not a Set<T>?
          try {
            m_parent.m_maker->flush(in.end_time, out_vec);
          } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
            ers::error(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
            continue;
          }
          break;
        case Set<A>::Type::kUnknown:
          ers::error(UnknownSetError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
          break;
      }
      
      while (out_vec.size() && running_flag.load()) {
        if (m_parent.send(out_vec.back())) {
          out_vec.pop_back();
        }
      }
    } // end while (running_flag.load())

    TLOG() << ": Exiting do_work() method, received " << m_parent.m_received_count 
           << " inputs and successfully sent " << m_parent.m_sent_count << " outputs. ";
  }
};

} // namespace dunedaq::trigger

#endif // TRIGGER_SRC_TRIGGER_TRIGGERGENERICMAKER_HPP_
