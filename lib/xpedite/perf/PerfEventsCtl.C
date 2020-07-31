/////////////////////////////////////////////////////////////////////////////////////////////////
//
// PerfEventsCtl - Logic to program and collect perf events.
//
//*********************************************************************************************
// Life cycle of perf events
//*********************************************************************************************
// 
// Construction of perf events
// - Each profiling session can optionally enable pmu counters using the perf events api
// - The events are allocated and stored in a map, with one perf event set for each thread.
// - The xpedite background thread will construct perf event set by opening file descriptors
//   and mapping memory for each of the known application threads.
// - For the threads that are spawned during or after enabling of perf events, the thread
//   itself is responsible for allocating it's perf event set in the map
// 
// Destruction of perf events
// - The perf events will be disabled (file descriptors closed, memory unmapped) at the end of
//   a profiling session.
// - The xpedite background thread will exchange the perf event set map with an empty one
//
// Race conditions:
// This pattern opens the possibility for the following race conditions
//
// #1 - A thread is spawned, while the events are being enabled. 
//      There exists a race between xpedite background thread and the newly spawned thread
//      to concurrently create perf events for the new thread.
//      
//      Generation count is used to sanity check and ensure the race results in activation of 
//      most recent events.
//      
// #2 - The xpedite background thread deactivates a perf event, while critical thread is active.
//      The ordering of actions carried out during a profile session de-activation, makes it
//      highly unlikely for such a race to happen.
//      As a second safety net, the release of de-activated events is delayed for cycle to live
//      duration, to provide ample time for all critical threads to exit probe trampolines.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/perf/PerfEventsCtl.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <xpedite/log/Log.H>
#include <xpedite/pmu/Formatter.h>

namespace xpedite { namespace perf {

  PerfEventsCtl::PerfEventsCtl()
    : _activeEventAttrs {}, _activeEvents {}, _mutex {}, _generation {}, _isEnabled {} {
  }

  void PerfEventsCtl::publishEventAttrs(const PerfEventAttrSet& eventAttrs_) noexcept {
    std::lock_guard<std::mutex> guard {_mutex};
    _activeEventAttrs = eventAttrs_;
    ++_generation;
  }

  std::tuple<uint64_t, PerfEventAttrSet> PerfEventsCtl::snapEventAttrs() const noexcept {
    uint64_t generation;
    PerfEventAttrSet eventAttrs;
    {
      std::lock_guard<std::mutex> guard {_mutex};
      generation = _generation;
      eventAttrs = _activeEventAttrs;
    }
    return std::make_tuple(generation, eventAttrs);
  }

  bool PerfEventsCtl::enable(const PerfEventAttrSet& eventAttrs_, PerfEventSetMap& inertEvents_) {
    if(_isEnabled) {
      XpediteLogCritical << "xpedite doen't support multiplexing perf events - generation " 
        << _generation << " already enabled" << XpediteLogEnd;
      return {};
    }

    if(!eventAttrs_) {
      XpediteLogCritical << "failed to enable empty pmu request" << XpediteLogEnd;
      return {};
    }

    publishEventAttrs(eventAttrs_);
    auto samplesBufferHead = framework::SamplesBuffer::head();

    std::vector<PerfEventSet> perfEventSets;
    auto buffer = samplesBufferHead;
    int bufferCount {};
    while(buffer) {
      PerfEventSet perfEventSet { buildPerfEvents(eventAttrs_, _generation, buffer->tid()) };
      if(perfEventSet.size() != eventAttrs_.size()) {
        return {};
      }
      perfEventSets.emplace_back(std::move(perfEventSet));
      buffer = buffer->next();
      ++bufferCount;
    }

    XpediteLogInfo << "enabling perf events for " << bufferCount << " threads\n" 
      << eventAttrs_.toString() << XpediteLogEnd;

    int i {};
    buffer = samplesBufferHead;
    {
      std::lock_guard<std::mutex> guard {_mutex};
      while(buffer) {
        auto inertEventSetPtr = attachUnsafe(buffer, std::move(perfEventSets.at(i++)));
        if(inertEventSetPtr && *inertEventSetPtr) {
          auto tid = inertEventSetPtr->tid();
          inertEvents_.emplace(std::make_pair(tid, std::move(inertEventSetPtr)));
        }
        buffer = buffer->next();
      }
    }
    return _isEnabled = true;
  }

  PerfEventsCtl::PerfEventSetPtr PerfEventsCtl::attachUnsafe(framework::SamplesBuffer* samplesBuffer_, PerfEventSet&& perfEventSet_) {
    if(perfEventSet_.tid() != samplesBuffer_->tid()) {
      throw std::runtime_error {"Invariant violation - detected thread mismatch in perf event set"};
    }

    if(!_activeEventAttrs) {
      return {};
    }

    auto& activeEventSetPtr = _activeEvents[perfEventSet_.tid()];
    if(!activeEventSetPtr || !*activeEventSetPtr || activeEventSetPtr->generation() < perfEventSet_.generation()) {
      PerfEventSetPtr perfEventSetPtr {new PerfEventSet {std::move(perfEventSet_)}};
      std::swap(activeEventSetPtr, perfEventSetPtr);
      samplesBuffer_->updatePerfEvents(activeEventSetPtr.get());
      return perfEventSetPtr;
    }
    return {};
  }

  bool PerfEventsCtl::attachTo(framework::SamplesBuffer* samplesBuffer_, PerfEventSetPtr& inertEventSetPtr_) {
    uint64_t generation;
    PerfEventAttrSet eventAttrs;
    std::tie(generation, eventAttrs) = snapEventAttrs();
    if(eventAttrs) {
      PerfEventSet perfEventSet { buildPerfEvents(eventAttrs, generation, samplesBuffer_->tid()) };
      if(perfEventSet.size() == eventAttrs.size()) {
        std::lock_guard<std::mutex> guard {_mutex};
        inertEventSetPtr_ = attachUnsafe(samplesBuffer_, std::move(perfEventSet));
        return true;
      }
      else {
        XpediteLogError << "xpedite - Failed to program pmu for thread - " << samplesBuffer_->tid()
          << " | event set - " << eventAttrs.toString() << XpediteLogEnd;
      }
    }
    return {};
  }

  PerfEventsCtl::PerfEventSetMap PerfEventsCtl::disable() noexcept {
    if(_isEnabled) {
      PerfEventSetMap perfEventSetMap {};
      {
        std::lock_guard<std::mutex> guard {_mutex};
        _activeEventAttrs = {};
        std::swap(_activeEvents, perfEventSetMap);
      }

      if(!perfEventSetMap.empty()) {
        for(auto& perfEventSetPair : perfEventSetMap) {
          perfEventSetPair.second->deactivate();
        }
      }
      _isEnabled = {};
      return perfEventSetMap;
    }
    return {};
  }

}}
