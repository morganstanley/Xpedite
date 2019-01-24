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

#pragma once
#include <xpedite/perf/PerfEvent.H>
#include <xpedite/perf/PerfEventAttrSet.H>
#include <unistd.h>
#include <array>

namespace xpedite { namespace perf {

  class PerfEventSet
  {
    std::array<PerfEvent, XPEDITE_PMC_CTRL_CORE_EVENT_MAX> _events;
    int _size;
    uint64_t _generation;
    bool _active;

    public:

    PerfEventSet()
      : _events {}, _size {}, _generation {}, _active {} {
    }

    explicit PerfEventSet(uint64_t generation_)
      : _events {}, _size {}, _generation {generation_}, _active {} {
    }

    PerfEventSet(const PerfEventSet& other_) noexcept = delete;
    PerfEventSet& operator=(const PerfEventSet& other_) noexcept = delete;

    PerfEventSet(PerfEventSet&& other_) noexcept
      : _events {std::move(other_._events)}, _size {other_._size}, _generation {other_._generation},
        _active {other_._active} {
        other_._size = {};
        other_._generation = {};
        other_._active = {};
    }

    PerfEventSet& operator=(PerfEventSet&& other_) noexcept {
      std::swap(_events, other_._events);
      std::swap(_size, other_._size);
      std::swap(_generation, other_._generation);
      std::swap(_active, other_._active);
      return *this;
    }

    ~PerfEventSet() {
      deactivate();
    }

    bool add(PerfEvent&& event_);

    bool add(perf_event_attr attr, pid_t tid_);

    bool activate();

    bool deactivate();

    int groupFd() const noexcept {
      return _size > 0 ? _events[0].fd() : -1;
    }

    perf_event_mmap_page* buffer() const noexcept {
      return _size > 0 ? _events[0].buffer() : nullptr;
    }

    pid_t tid() const noexcept {
      return _size > 0 ? _events[0].tid() : 0;
    }

    bool isActive() {
      return _active;
    }

    void read(uint64_t* buffer_) const noexcept {
      for(int i=0; i<_size; ++i) {
        buffer_[i] = _events[i].read();
      }
    }

    int size() const noexcept {
      return _size;
    }

    uint64_t generation() const noexcept {
      return _generation;
    }

    explicit operator bool() const {
      return _size > 0;
    }
  };

  PerfEventSet buildPerfEvents(const PerfEventAttrSet& eventAttrs_, uint64_t generation_, pid_t tid_);
}}
