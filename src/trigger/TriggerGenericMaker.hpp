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
#include "trigger/TimeSliceInputBuffer.hpp"
#include "trigger/TimeSliceOutputBuffer.hpp"
 
#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "dataformats/GeoID.hpp"

#include "logging/Logging.hpp"

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace dunedaq::trigger {

// Forward declare the class encapsulating partial specifications of do_work
template<class IN, class OUT, class MAKER>
class TriggerGenericWorker;

// This template class reads IN items from queues, passes them to MAKER objects,
// and writes the resulting OUT objects to another queue. The behavior of 
// passing IN objects to the MAKER and creating OUT objects from the MAKER is 
// encapsulated by TriggerGenericWorker<IN,OUT,MAKER> templates, defined later
// in this file
template<class IN, class OUT, class MAKER>
class TriggerGenericMaker : public dunedaq::appfwk::DAQModule
{
friend class TriggerGenericWorker<IN,OUT,MAKER>;
  
public:

  explicit TriggerGenericMaker(const std::string& name)
    : DAQModule(name)
    , m_thread(std::bind(&TriggerGenericMaker::do_work, this, std::placeholders::_1))
    , m_input_queue(nullptr)
    , m_output_queue(nullptr)
    , m_queue_timeout(100)
    , m_algorithm_name("[uninitialized]")
    , m_geoid_region_id(dunedaq::dataformats::GeoID::s_invalid_region_id)
    , m_geoid_element_id(dunedaq::dataformats::GeoID::s_invalid_element_id)
    , m_buffer_time(0)
    , worker(*this) // should be last; may use other members
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

  // Only applies to makers that output Set<B>
  void set_geoid(uint16_t region_id, uint32_t element_id)
  {
    m_geoid_region_id = region_id;
    m_geoid_element_id = element_id;
  }
  
  // Only applies to makers that output Set<B>
  void set_buffer_time(dataformats::timestamp_t buffer_time)
  {
    m_buffer_time = buffer_time;
  }

private:

  dunedaq::appfwk::ThreadHelper m_thread;
  
  size_t m_received_count;
  size_t m_sent_count;
  
  using source_t = dunedaq::appfwk::DAQSource<IN>;
  std::unique_ptr<source_t> m_input_queue;

  using sink_t = dunedaq::appfwk::DAQSink<OUT>;
  std::unique_ptr<sink_t> m_output_queue;

  std::chrono::milliseconds m_queue_timeout;
  
  std::string m_algorithm_name;
  
  uint16_t m_geoid_region_id;
  uint32_t m_geoid_element_id;

  dataformats::timestamp_t m_buffer_time;

  std::shared_ptr<MAKER> m_maker;
  
  TriggerGenericWorker<IN,OUT,MAKER> worker;
  
  // This should return a shared_ptr to the MAKER created from conf command arguments. 
  // Should also call set_algorithm_name and set_geoid/buffer_time (if desired)
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
    // worker should be notified that configuration potentially changed
    worker.reconfigure();
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
  
  void reconfigure()
  {
  }
  
  void do_work(std::atomic<bool> &running_flag)
  {
    while (running_flag.load()) {
      IN in;
      if (!m_parent.receive(in)) {
        continue; //nothing to do
      }
  
      std::vector<OUT> out_vec; // one input -> many outputs
      try {
        m_parent.m_maker->operator()(in, out_vec);
      } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
        ers::fatal(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
        return;
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



// Partial specialization for IN = Set<A>, OUT = Set<B> and assumes the MAKER has:
// operator()(A, std::vector<B>)
template<class A, class B, class MAKER> 
class TriggerGenericWorker<Set<A>, Set<B>, MAKER> 
{
public:
  explicit TriggerGenericWorker(TriggerGenericMaker<Set<A>, Set<B>, MAKER> &parent) 
    : m_parent(parent)
    , m_in_buffer(parent.get_name(), parent.m_algorithm_name)
    , m_out_buffer(parent.get_name(), parent.m_algorithm_name, parent.m_buffer_time)
  { }
  
  TriggerGenericMaker<Set<A>, Set<B>, MAKER> &m_parent;
  
  TimeSliceInputBuffer<A> m_in_buffer;
  TimeSliceOutputBuffer<B> m_out_buffer;
  
  void reconfigure()
  {
    m_out_buffer.set_buffer_time(m_parent.m_buffer_time);
  }
  
  void do_work(std::atomic<bool> &running_flag)
  {
    while (running_flag.load()) {
      Set<A> in;
      if (!m_parent.receive(in)) {
        continue; //nothing to do
      }

      std::vector<B> elems; //Bs to buffer for the next window
      switch (in.type) {
        case Set<A>::Type::kPayload:
          {
            std::vector<A> time_slice;
            dataformats::timestamp_t start_time, end_time;
            if (!m_in_buffer.buffer(in, time_slice, start_time, end_time)) {
              continue; //no complete time slice yet (`in` was part of buffered slice)
            }
            // time_slice is a full slice (all Set<A> combined), time ordered, vector of A
            // call operator for each of the objects in the vector
            for (A &x : time_slice) {
              std::vector<B> out_vec;
              try {
                m_parent.m_maker->operator()(x, out_vec);
              } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
                ers::fatal(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
                return;
              }
              elems.insert(elems.end(), out_vec.begin(), out_vec.end());
            }
            //register this window
            // TODO BJL July 14-2021 this must be called for each possible window emitted as Set<B>
            // but will all Set<B> fall within a window of Set<A>? Is there a better way to know
            // what windows to allow than what windows we receive?
            m_out_buffer.add_window(start_time, end_time);
          }
          break;
        case Set<A>::Type::kHeartbeat:
          { 
            // forward the heartbeat
            Set<B> heartbeat;
            heartbeat.seqno = m_parent.m_sent_count;
            heartbeat.type = Set<B>::Type::kHeartbeat;
            heartbeat.start_time = in.start_time;
            heartbeat.end_time = in.end_time;
            heartbeat.origin = dataformats::GeoID(dataformats::GeoID::SystemType::kDataSelection, 
                                                  m_parent.m_geoid_region_id,
                                                  m_parent.m_geoid_element_id);
            while (running_flag.load()) {
              if (m_parent.send(heartbeat)) {
                break;
              }
            }
            // flush the maker
            try {
              // TODO BJL July 14-2021 flushed events go into the buffer... until a window is ready?
              m_parent.m_maker->flush(in.end_time, elems);
            } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
              ers::fatal(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
              return;
            }
          }
          break;
        case Set<A>::Type::kUnknown:
          ers::error(UnknownSetError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
          break;
      }
          
      // add new elements to output buffer
      if (elems.size() > 0) {
        m_out_buffer.buffer(elems);
      }
      
      // emit completed windows
      while (m_out_buffer.ready()) {
        Set<B> out;
        out.seqno = m_parent.m_sent_count;
        out.type = Set<B>::Type::kPayload;
        m_out_buffer.flush(out.objects, out.start_time, out.end_time);
        out.origin = dataformats::GeoID(dataformats::GeoID::SystemType::kDataSelection, 
                                        m_parent.m_geoid_region_id,
                                        m_parent.m_geoid_element_id);
        TLOG() << "Output window ready with start time " << out.start_time 
               << " end time " << out.end_time << " and " 
               << out.objects.size() << " members";
        if (out.objects.size() == 0) {
          TLOG() << "Suppressing empty window";
          // out discarded
        } else {
          while (running_flag.load()) {
            if (m_parent.send(out)) {
              break;
            }
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
  explicit TriggerGenericWorker(TriggerGenericMaker<Set<A>, OUT, MAKER> &parent) 
    : m_parent(parent)
    , m_in_buffer(parent.get_name(), parent.m_algorithm_name)
  { }
  
  TriggerGenericMaker<Set<A>, OUT, MAKER> &m_parent;
  
  TimeSliceInputBuffer<A> m_in_buffer;  
  
  void reconfigure()
  {
  }
  
  void do_work(std::atomic<bool> &running_flag)
  {
    while (running_flag.load()) {
      Set<A> in;
      if (!m_parent.receive(in)) {
        continue; //nothing to do
      }

      std::vector<OUT> out_vec; // either a whole time slice, heartbeat flushed, or empty
      switch (in.type) {
        case Set<A>::Type::kPayload:   
          {
            std::vector<A> time_slice;
            dataformats::timestamp_t start_time, end_time;
            if (!m_in_buffer.buffer(in, time_slice, start_time, end_time)) {
              continue; //no complete time slice yet (`in` was part of buffered slice)
            }
            // time_slice is a full slice (all Set<A> combined), time ordered, vector of A
            // call operator for each of the objects in the vector
            for (A &x : time_slice) {
              try {
                m_parent.m_maker->operator()(x, out_vec);
              } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
                ers::fatal(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
                return;
              }
            }
          }
          break;
        case Set<A>::Type::kHeartbeat:
          // TODO BJL May-28-2021 should anything happen with the heartbeat when OUT is not a Set<T>?
          try {
            m_parent.m_maker->flush(in.end_time, out_vec);
          } catch (...) { // TODO BJL May 28-2021 can we restrict the possible exceptions triggeralgs might raise?
            ers::fatal(AlgorithmFatalError(ERS_HERE, m_parent.get_name(), m_parent.m_algorithm_name));
            return;
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
