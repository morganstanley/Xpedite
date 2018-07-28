///////////////////////////////////////////////////////////////////////////////
//
// PMUCtl - Logic to program and collect core, fixed and offcore performance
// counters
//
// Supports two methods of programming and collection pmu events
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
#include <xpedite/util/Errno.H>
#include <linux/perf_event.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

extern "C" int perf_event_open(struct perf_event_attr *hwEvent_, pid_t pid_, int cpu_, int groupFd_, unsigned long flags_) {
  return syscall(__NR_perf_event_open, hwEvent_, pid_, cpu_, groupFd_, flags_);
}

namespace xpedite { namespace pmu {

  PmuCtl PmuCtl::_instance;

  PmuCtl::PmuCtl()
    : _generation {}, _eventSelect {}, _genericPmcCount {}, _fixedPmcSet {} {
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

  perf_event_attr buildPerfEventAttr(uint32_t eventSelect_) {
    perf_event_attr attr {};
    attr.type = PERF_TYPE_RAW;
    attr.size = sizeof(attr);
    attr.config = static_cast<uint16_t>(eventSelect_);
    attr.disabled = 1;
    return attr;
  }

  void PmuCtl::publishEventSelect(const EventSelect& eventSelect_) noexcept {
    _generation.fetch_add(1, std::memory_order_seq_cst);
    _eventSelect = eventSelect_;
    _generation.fetch_add(1, std::memory_order_seq_cst);
  }

  int PmuCtl::snapEventSelect(EventSelect& eventSelect_) const noexcept {
    auto generation = _generation.load(std::memory_order_acquire);
    if(generation % 2) {
      return -1;
    }
    eventSelect_ = _eventSelect;
    if(generation == _generation.load(std::memory_order_seq_cst)) {
      return generation;
    }
    return -1;
  }

  bool PmuCtl::enable(const PMUCtlRequest& request_) {

    EventSelect eventSelect {};

    if(!::buildEventSelect(&request_, &eventSelect)) {
      publishEventSelect(eventSelect);
    }
    else {
      XpediteLogCritical << "failed to decode pmu request" << XpediteLogEnd;
      return {};
    }

    _genericPmcCount = request_._gpEvtCount;
    for(uint8_t i=0; i< request_._fixedEvtCount; ++i) {
      _fixedPmcSet.enable(request_._fixedEvents[i]._ctrIndex);
    }

    auto samplesBuffer = framework::SamplesBuffer::head();
    std::vector<EventSet> eventSets;
    auto buffer = samplesBuffer;
    while(buffer) {
      EventSet eventSet; bool rc;
      std::tie(eventSet, rc) = buildEventSet(eventSelect, _generation.load(std::memory_order_relaxed));
      if(!rc) {
        return {};
      }
      eventSets.emplace_back(std::move(eventSet));
      buffer = buffer->next();
    }

    int i {};
    buffer = samplesBuffer;
    while(buffer) {
      buffer->updateEventSet(std::move(eventSets.at(i++)));
      buffer = buffer->next();
    }
    probes::recorderCtl().activateRecorder(probes::RecorderType::EVENT_SET_RECORDER);
    return true;
  }

  std::tuple<EventSet, bool> PmuCtl::buildEventSet(const EventSelect& eventSelect_, int generation_) noexcept {
    EventSet eventSet {generation_};
    for(int i=0; i < eventSelect_._gpEvtCount; ++i) {
      auto attr = buildPerfEventAttr(eventSelect_._gpEvtSel[i]);
      auto fd = perf_event_open(&attr, 0, -1, -1, 0);
      if (fd == -1) {
        xpedite::util::Errno err;
        XpediteLogCritical << "failed to open pmu event (" << attr.config << ") - " << err.asString() << XpediteLogEnd;
        return {};
      }

      auto handle = reinterpret_cast<perf_event_mmap_page*>(mmap(nullptr, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
      if(handle == MAP_FAILED) {
        xpedite::util::Errno err;
        XpediteLogCritical << "failed to map pmu event (" << attr.config << ") - " << err.asString() << XpediteLogEnd;
        return {};
      }

      eventSet.add(Event {fd, handle});

      if(ioctl(fd, PERF_EVENT_IOC_RESET, 0) || ioctl(fd, PERF_EVENT_IOC_ENABLE, 0)) {
        xpedite::util::Errno err;
        XpediteLogCritical << "failed to activate pmu event (" << attr.config << ") - " << err.asString() << XpediteLogEnd;
        return {};
      }
    }
    return std::make_tuple(std::move(eventSet), true);
  }

  void PmuCtl::disable() noexcept {
    disableGenericPmc();
    disableFixedPmc();
  }

}}
