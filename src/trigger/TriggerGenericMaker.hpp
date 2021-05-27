/**
 * @file TriggerGenericMaker.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
 
#ifndef TRIGGER_INCLUDE_TRIGGER_TRIGGERGENERICMAKER_HPP_
#define TRIGGER_INCLUDE_TRIGGER_TRIGGERGENERICMAKER_HPP_

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

namespace dunedaq::trigger {

// Forward declare the class encapsulating partial specifications of do_work
template<class IN, class OUT, class MAKER>
class Worker;

template<class IN, class OUT, class MAKER>
class TriggerGenericMaker : public dunedaq::appfwk::DAQModule
{
friend class Worker<IN,OUT,MAKER>;
  
public:

  explicit TriggerGenericMaker(const std::string& name)
    : DAQModule(name)
    , worker(*this)
    , m_thread(std::bind(&TriggerGenericMaker::do_work, this, std::placeholders::_1))
    , m_input_queue(nullptr)
    , m_output_queue(nullptr)
    , m_queue_timeout(100)
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

private:

  Worker<IN,OUT,MAKER> worker;

  dunedaq::appfwk::ThreadHelper m_thread;
  
  using source_t = dunedaq::appfwk::DAQSource<IN>;
  std::unique_ptr<source_t> m_input_queue;

  using sink_t = dunedaq::appfwk::DAQSink<OUT>;
  std::unique_ptr<sink_t> m_output_queue;

  std::chrono::milliseconds m_queue_timeout;

  std::shared_ptr<MAKER> m_maker;
  
  virtual std::shared_ptr<MAKER> make_maker(const nlohmann::json& obj) = 0;
  
  void do_start(const nlohmann::json& /*obj*/)
  {
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

};

// To handle the different unpacking schemes implied by different templates,
// do_work is broken out into its own template class that is a friend of 
// TriggerGenericMaker. C++ still does not support partial specification of a 
// single method in a template class, so this approach is the least redundant
// way to achieve that functionality
template<class IN, class OUT, class MAKER>
class Worker
{
public:
  Worker(TriggerGenericMaker<IN, OUT, MAKER> &_parent) : parent(_parent) { }
  
  TriggerGenericMaker<IN, OUT, MAKER> &parent;
  
  void do_work(std::atomic<bool> &running_flag)
  {
    int received_count = 0;
    int sent_count = 0;

    while (running_flag.load()) {
      IN in;
      try {
        parent.m_input_queue->pop(in, parent.m_queue_timeout);
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        // it is perfectly reasonable that there might be no data in the queue
        // some fraction of the times that we check, so we just continue on and try again
        continue;
      }
      ++received_count;
  
      std::vector<OUT> outs;
      try {
        parent.m_maker->operator()(in, outs);
      } catch (...) {
        ers::error(AlgorithmFatalError(ERS_HERE, parent.get_name(), "algorithm_name_placeholder"));
        continue;
      }

      while (outs.size() && running_flag.load()) {
        try {
          parent.m_output_queue->push(outs.back(), parent.m_queue_timeout);
          outs.pop_back();
          ++sent_count;
        } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
          ers::warning(excpt);
        }
      }
    } // end while (running_flag.load())

    TLOG() << ": Exiting do_work() method, received " << received_count 
           << " inputs and successfully sent " << sent_count << " outputs. ";
  }
  
};

// Partial specilization for Set<A> -> Set<B>
template<class A, class B, class MAKER> 
class Worker<Set<A>, Set<B>, MAKER> 
{
public:
  Worker(TriggerGenericMaker<Set<A>, Set<B>, MAKER> &_parent) : parent(_parent) { }
  
  TriggerGenericMaker<Set<A>, Set<B>, MAKER> &parent;
  
  void do_work(std::atomic<bool> &running_flag)
  {
    int received_count = 0;
    int sent_count = 0;

    while (running_flag.load()) {
      Set<A> in;
      try {
        parent.m_input_queue->pop(in, parent.m_queue_timeout);
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        // it is perfectly reasonable that there might be no data in the queue
        // some fraction of the times that we check, so we just continue on and try again
        continue;
      }
      ++received_count;

      Set<B> out;
      switch (in.type) {
        case Set<A>::Type::kPayload:
          for (size_t i = 0; i < in.objects.size(); i++) {
            std::vector<B> out_vec;
            try {
              parent.m_maker->operator()(in.objects[i], out_vec);
            } catch (...) {
              ers::error(AlgorithmFatalError(ERS_HERE, parent.get_name(), "algorithm_name_placeholder"));
              continue;
            }
            out.objects.insert(out.objects.end(), out_vec.begin(), out_vec.end());
          }
          out.type = Set<B>::Type::kPayload;
          break;
        case Set<A>::Type::kHeartbeat:
          // FIXME BJL 5-27-21 do we flush the m_maker here?
          out.type = Set<B>::Type::kHeartbeat;
          break;
        case Set<A>::Type::kUnknown:
          ers::error(UnknownSetError(ERS_HERE, parent.get_name(), "algorithm_name_placeholder"));
          break;
      }
          
      out.seqno = sent_count;
      if (out.objects.size() > 0) {
        // FIXME BJL 5-27-21 Set<T> should implement some helper methods to 
        // populate these fields in lieu of that, this is an approximation
        out.from_detids.resize(out.objects.size());
        for (size_t i = 0; i < out.objects.size(); i++) {
          out.from_detids[i] = out.objects[i].detid;
        }
        out.start_time = out.objects[0].time_start;
        // TODO BJL 5-27-21 TP and TA both have time_start, but only TA has time_end
        // really, Set<T> should decide what is desired here, not the maker
        out.end_time = out.objects[out.objects.size()-1].time_start;
      }
        
      try {
        parent.m_output_queue->push(out, parent.m_queue_timeout);
        ++sent_count;
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        ers::warning(excpt);
      }
    } // end while (running_flag.load())

    TLOG() << ": Exiting do_work() method, received " << received_count 
           << " inputs and successfully sent " << sent_count << " outputs. ";
  }
  
};

// Partial specilization for Set<A> -> OUT
template<class A, class OUT, class MAKER> 
class Worker<Set<A>, OUT, MAKER> 
{
public:
  Worker(TriggerGenericMaker<Set<A>, OUT, MAKER> &_parent) : parent(_parent) { }
  
  TriggerGenericMaker<Set<A>, OUT, MAKER> &parent;
  
  void do_work(std::atomic<bool> &running_flag)
  {
    int received_count = 0;
    int sent_count = 0;

    while (running_flag.load()) {
      Set<A> in;
      try {
        parent.m_input_queue->pop(in, parent.m_queue_timeout);
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        // it is perfectly reasonable that there might be no data in the queue
        // some fraction of the times that we check, so we just continue on and try again
        continue;
      }
      ++received_count;

      Set<OUT> out;
      switch (in.type) {
        case Set<A>::Type::kPayload:
          for (size_t i = 0; i < in.objects.size(); i++) {
            std::vector<OUT> out_vec;
            try {
              parent.m_maker->operator()(in.objects[i], out_vec);
            } catch (...) {
              ers::error(AlgorithmFatalError(ERS_HERE, parent.get_name(), "algorithm_name_placeholder"));
            }
            while (out_vec.size() && running_flag.load()) {
              try {
                parent.m_output_queue->push(out_vec.back(), parent.m_queue_timeout);
                out_vec.pop_back();
                ++sent_count;
              } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
                ers::warning(excpt);
              }
            }
          }
          break;
        case Set<A>::Type::kHeartbeat:
          // FIXME BJL 5-27-21 do we flush the m_maker here? Forward payload?
          break;
        case Set<A>::Type::kUnknown:
          ers::error(UnknownSetError(ERS_HERE, parent.get_name(), "algorithm_name_placeholder"));
          break;
      }
          
    } // end while (running_flag.load())

    TLOG() << ": Exiting do_work() method, received " << received_count 
           << " inputs and successfully sent " << sent_count << " outputs. ";
  }
  
};

} // namespace dunedaq::trigger

#endif // TRIGGER_INCLUDE_TRIGGER_TRIGGERGENERICMAKER_HPP_
