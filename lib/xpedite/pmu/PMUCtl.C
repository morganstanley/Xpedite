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
#include <xpedite/log/Log.H>
#include <xpedite/pmu/Formatter.h>

namespace xpedite { namespace pmu {

  PmuCtl* PmuCtl::_instance {};

  uint64_t PmuCtl::_quiesceDuration {PmuCtl::DEFAULT_QUIESCE_DURATION};

  PmuCtl::PmuCtl()
    : _inertEventsQueue {}, _genericPmcCount {}, _fixedPmcSet {} {
  }

  void PmuCtl::enableGenericPmc(uint8_t genericPmcCount_) noexcept {
    if(!genericPmcCount_) {
      return;
    }
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

  bool PmuCtl::enablePerfEvents(const PMUCtlRequest& request_) {

    EventSet eventSet {};
    if(::buildEventSet(&request_, &eventSet)) {
      XpediteLogCritical << "failed to decode pmu request" << XpediteLogEnd;
      return {};
    }
    logEventSet(&request_, &eventSet);

    PerfEventSetMap perfEventSetMap {};
    auto eventAttrs = perf::buildPerfEventAttrs(eventSet);

    if(PerfEventsCtl::enable(eventAttrs, perfEventSetMap)) {
      if(!perfEventSetMap.empty()) {
        _inertEventsQueue.emplace_back(std::move(perfEventSetMap), generation()-1);
      }

      _genericPmcCount = request_._gpEvtCount;
      for(uint8_t i=0; i< request_._fixedEvtCount; ++i) {
        _fixedPmcSet.enable(request_._fixedEvents[i]._ctrIndex);
      }
      probes::recorderCtl().activateRecorder(probes::RecorderType::PERF_EVENTS_RECORDER);
      return true;
    }
    return {};
  }

  bool PmuCtl::attachPerfEvents(framework::SamplesBuffer* samplesBuffer_) {
    PerfEventSetPtr inertEventSetPtr {};
    return PerfEventsCtl::attachTo(samplesBuffer_, inertEventSetPtr);
  }

  void PmuCtl::disablePerfEvents() noexcept {
    disableGenericPmc();
    disableFixedPmc();
    auto perfEventSetMap = PerfEventsCtl::disable();
    if(!perfEventSetMap.empty()) {
      _inertEventsQueue.emplace_back(std::move(perfEventSetMap), generation());
      XpediteLogInfo << "xpedite - Enqueued perf event set [generation - " << generation() << " | threads - "
        << perfEventSetMap.size() << "] for recycling" << XpediteLogEnd;
    }
  }

  void PmuCtl::poll() {
    auto expiryTsc = RDTSC() - _quiesceDuration;
    for(auto it = _inertEventsQueue.cbegin(); it != _inertEventsQueue.cend();) {
      if(it->_tsc < expiryTsc) {
        XpediteLogInfo << "xpedite - Releasing expired perf event set [generation - " << generation() << " | threads - "
          << it->_events.size() << "]" << XpediteLogEnd;
        it = _inertEventsQueue.erase(it);
      }
      else {
        break;
      }
    }
  }

}}
