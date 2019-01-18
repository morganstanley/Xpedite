///////////////////////////////////////////////////////////////////////////////
//
// PMUCtl - Logic to program and collect core, fixed and offcore performance
// counters
//
// Supports programming and collection of pmu events using
// 1. RDPMC with events programmed out of band by xpedite kernel module
// 2. RDPMC with events programmed using the linux perf events api
//
// Programming and collecting using xpedite kernel module has less overhead
// compared to using the perf events api
//
// Enabling/disabling pmu events, automatically selects appropriate recorders
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/probes/RecorderCtl.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <xpedite/log/Log.H>
#include <xpedite/pmu/Formatter.h>

namespace xpedite { namespace pmu {

  PmuCtl PmuCtl::_instance;

  uint64_t PmuCtl::_quiesceDuration {PmuCtl::DEFAULT_QUIESCE_DURATION};

  PmuCtl::PmuCtl()
    : _activeEventAttrs {}, _activeEvents {}, _inertEventsQueue {}, _mutex {},
    _generation {}, _perfEventsEnabled {}, _genericPmcCount {}, _fixedPmcSet {} {
  }

  void PmuCtl::enableGenericPmc(uint8_t genericPmcCount_) noexcept {
    if(pmcCount() == 0) {
      probes::recorderCtl().activateRecorder(probes::RecorderType::PMC_RECORDER);
    }
    _genericPmcCount = genericPmcCount_;
  }

  void PmuCtl::disableGenericPmc() noexcept {
    if(_genericPmcCount) {
      _genericPmcCount = 0;
      if(pmcCount() == 0) {
        probes::recorderCtl().activateRecorder(probes::RecorderType::EXPANDABLE_RECORDER);
      }
    }
  }

  void PmuCtl::enableFixedPmc(uint8_t index_) noexcept {
    if(pmcCount() == 0) {
      probes::recorderCtl().activateRecorder(probes::RecorderType::PMC_RECORDER);
    }
    _fixedPmcSet.enable(index_);
  }

  void PmuCtl::disableFixedPmc() noexcept {
    if(_fixedPmcSet.size()) {
      _fixedPmcSet.reset();
      if(pmcCount() == 0) {
        probes::recorderCtl().activateRecorder(probes::RecorderType::EXPANDABLE_RECORDER);
      }
    }
  }

  void PmuCtl::publishEventAttrs(const perf::PerfEventAttrSet& eventAttrs_) noexcept {
    std::lock_guard<std::mutex> guard {_mutex};
    _activeEventAttrs = eventAttrs_;
  }

  perf::PerfEventAttrSet PmuCtl::snapEventAttrs() const noexcept {
    PerfEventAttrSet eventAttrs;
    {
      std::lock_guard<std::mutex> guard {_mutex};
      eventAttrs = _activeEventAttrs;
    }
    return eventAttrs;
  }

  /**********************************************************************************************
   * Life cycle of perf events
   * 
   * Construction of perf events
   * - Each profiling session can optionally enable pmu counters using the perf events api
   * - The events are allocated and stored in a map, with one perf event set for each thread.
   * - The xpedite background thread will construct perf event set by opening file descriptors
   *   and mapping memory for each of the known application threads.
   * - For the threads that are spawned during or after enabling of perf events, the thread
   *   itself is responsible for allocating it's perf event set in the map
   * 
   * Destruction of perf events
   * - The perf events will be disabled (file descriptors closed, memory unmapped) at the end of
   *   a profiling session.
   * - The xpedite background thread will exchange the perf event set map with an empty one
   *
   * Race conditions:
   * This pattern opens the possibility for the following race conditions
   *
   * #1 - A thread is spawned, while the events are being enabled. 
   *      There exists a race between xpedite background thread and the newly spawned thread
   *      to concurrently create perf events for the new thread.
   *      
   *      Generation count is used to sanity check and ensure the race results in activation of 
   *      most recent events.
   *      
   * #2 - The xpedite background thread deactivates a perf event, while critical thread is active.
   *      The ordering of actions carried out during a profile session de-activation, makes it
   *      highly unlikely for such a race to happen.
   *      As a second safety net, the release of de-activated events is delayed for cycle to live
   *      duration, to provide ample time for all critical threads to exit probe trampolines.
   **********************************************************************************************/

  bool PmuCtl::enablePerfEvents(const PMUCtlRequest& request_) {
    if(_perfEventsEnabled) {
      XpediteLogCritical << "xpedite doen't support multiplexing perf events - generation " 
        << _generation << " already enabled" << XpediteLogEnd;
      return {};
    }

    EventSet eventSet {};
    if(::buildEventSet(&request_, &eventSet)) {
      XpediteLogCritical << "failed to decode pmu request" << XpediteLogEnd;
      return {};
    }

    logEventSet(&request_, &eventSet);
    PerfEventAttrSet eventAttrs {perf::buildPerfEventAttrs(++_generation, eventSet)};
    if(!eventAttrs) {
      XpediteLogCritical << "failed to enable empty pmu request" << XpediteLogEnd;
      return {};
    }

    publishEventAttrs(eventAttrs);
    auto samplesBufferHead = framework::SamplesBuffer::head();

    std::vector<PerfEventSet> perfEventSets;
    auto buffer = samplesBufferHead;
    int bufferCount {};
    while(buffer) {
      PerfEventSet perfEventSet { buildPerfEvents(eventAttrs, buffer->tid()) };
      if(perfEventSet.size() != eventAttrs.size()) {
        return {};
      }
      perfEventSets.emplace_back(std::move(perfEventSet));
      buffer = buffer->next();
      ++bufferCount;
    }

    XpediteLogInfo << "enabling perf events for " << bufferCount << " threads\n" 
      << eventAttrs.toString() << XpediteLogEnd;

    int i {};
    buffer = samplesBufferHead;
    PerfEventSetMap perfEventSetMap {};
    {
      std::lock_guard<std::mutex> guard {_mutex};
      while(buffer) {
        auto inertEventSetPtr = attachEventsUnsafe(buffer, std::move(perfEventSets.at(i++)));
        if(inertEventSetPtr && *inertEventSetPtr) {
          auto tid = inertEventSetPtr->tid();
          perfEventSetMap.emplace(std::make_pair(tid, std::move(inertEventSetPtr)));
        }
        buffer = buffer->next();
      }
    }
    if(!perfEventSetMap.empty()) {
      _inertEventsQueue.emplace_back(std::move(perfEventSetMap), _generation-1);
    }

    _genericPmcCount = request_._gpEvtCount;
    for(uint8_t i=0; i< request_._fixedEvtCount; ++i) {
      _fixedPmcSet.enable(request_._fixedEvents[i]._ctrIndex);
    }
    probes::recorderCtl().activateRecorder(probes::RecorderType::PERF_EVENTS_RECORDER);
    _perfEventsEnabled = true;
    return true;
  }

  PmuCtl::PerfEventSetPtr PmuCtl::attachEventsUnsafe(framework::SamplesBuffer* samplesBuffer_, PerfEventSet&& perfEventSet_) {
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
      return std::move(perfEventSetPtr);
    }
    return {};
  }

  bool PmuCtl::attachPerfEvents(framework::SamplesBuffer* samplesBuffer_) {
    PerfEventAttrSet eventAttrs {snapEventAttrs()};
    if(eventAttrs) {
      PerfEventSet perfEventSet { buildPerfEvents(eventAttrs, samplesBuffer_->tid()) };
      PerfEventSetPtr inertEventSetPtr {};
      if(perfEventSet.size() == eventAttrs.size()) {
        std::lock_guard<std::mutex> guard {_mutex};
        inertEventSetPtr = attachEventsUnsafe(samplesBuffer_, std::move(perfEventSet));
        return true;
      }
      else {
        XpediteLogError << "xpedite - Failed to program pmu for thread - " << samplesBuffer_->tid()
          << " | event set - " << eventAttrs.toString() << XpediteLogEnd;
      }
    }
    return {};
  }

  void PmuCtl::disablePerfEvents() noexcept {
    disableGenericPmc();
    disableFixedPmc();
    if(_perfEventsEnabled) {
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
        _inertEventsQueue.emplace_back(std::move(perfEventSetMap), _generation);
        XpediteLogInfo << "xpedite - Enqueued perf event set [generation - " << _generation << " | threads - "
          << perfEventSetMap.size() << "] for recycling" << XpediteLogEnd;
      }
      _perfEventsEnabled = {};
    }
  }

  void PmuCtl::poll() {
    auto expiryTsc = RDTSC() - _quiesceDuration;
    for(auto it = _inertEventsQueue.cbegin(); it != _inertEventsQueue.cend();) {
      if(it->_tsc < expiryTsc) {
        XpediteLogInfo << "xpedite - Releasing expired perf event set [generation - " << _generation << " | threads - "
          << it->_events.size() << "]" << XpediteLogEnd;
        it = _inertEventsQueue.erase(it);
      }
      else {
        break;
      }
    }
  }

}}
