///////////////////////////////////////////////////////////////////////////////
//
// Logic to program a group of pmu events using linux perf events api
//
// PerfEventSet is a collection of pmu perf events, that are programmed
// and collected as a group.
//  
// The events in a set must belong to same group / target thread
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/perf/PerfEventSet.H>
#include <xpedite/perf/PerfEventsApi.H>
#include <xpedite/util/Errno.H>
#include <xpedite/log/Log.H>
#include <stdexcept>

namespace xpedite { namespace perf {

  bool PerfEventSet::activate() {
    if(!_active && size()) {
      _active = perfEventsApi()->reset(groupFd()) && perfEventsApi()->enable(groupFd());
    }
    return _active;
  }

  bool PerfEventSet::deactivate() {
    if(_active && size()) {
      _active = !perfEventsApi()->disable(groupFd());
    }
    return !_active;
  }


  bool PerfEventSet::add(PerfEvent&& event_) {
    if(static_cast<unsigned>(_size) >= _events.size()) {
      throw std::runtime_error {"Invariant violation - perf event set exceeded max supported events"};
    }
    if(_size > 0 && tid() != event_.tid()) {
      throw std::runtime_error {"Invariant violation - detected grouping of events across threads"};
    }
    _events[_size++] = std::move(event_);
    return true;
  }

  bool PerfEventSet::add(perf_event_attr attr, pid_t tid_) {
    PerfEvent event {attr, tid_, groupFd()};
    if(event) {
      return add(std::move(event));
    }
    return {};
  }

  PerfEventSet buildPerfEvents(const PerfEventAttrSet& eventAttrs_, uint64_t generation_, pid_t tid_) {
    PerfEventSet perfEventSet {generation_};

    for(int i=0; i < eventAttrs_._size; ++i) {
      if(!perfEventSet.add(eventAttrs_._values[i], tid_)) {
        return {};
      }
    }

    if(!perfEventSet.activate()) {
      xpedite::util::Errno err;
      XpediteLogCritical << "failed to activate pmu event group fd (" << perfEventSet.groupFd() << ") - " << err.asString() << XpediteLogEnd;
      return {};
    }
    return perfEventSet;
  }

}}

