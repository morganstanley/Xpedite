///////////////////////////////////////////////////////////////////////////////
//
// Logic to program pmu events using linux perf events api
//
// Event - Abstraction for reading h/w pmc using perf events api
//
// EventSet - A set of perf events, programmed and collected as a group
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include "EventSelect.h"
#include <xpedite/platform/Builtins.H>
#include <xpedite/util/Tsc.H>
#include <unistd.h>
#include <linux/perf_event.h>
#include <array>
#include <algorithm>

namespace xpedite { namespace pmu {

  class Event
  {
    int _fd;
    perf_event_mmap_page* _handle;

    public:

    Event() noexcept
      : _fd {}, _handle {} {
    }

    Event(int fd_, perf_event_mmap_page* handle_) noexcept 
      : _fd {fd_}, _handle {handle_} {
    }

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    Event(Event&& other_) noexcept
      : _fd {other_._fd}, _handle {other_._handle} {
      other_._fd = {};
      other_._handle = {};
    }

    Event& operator=(Event&& other_) noexcept {
      std::swap(_fd, other_._fd);
      std::swap(_handle, other_._handle);
      return *this;
    }

    ~Event() noexcept {
      //unmap handle
      if(_handle) {
      }
      if(_fd) {
        close(_fd);
      }
    }

    uint64_t read() const noexcept {
      uint32_t seq;
      do {
          seq = _handle->lock;
          common::compilerBarrier();
          if (auto idx = _handle->index) {
            return _handle->offset + RDPMC(idx - 1);
          }
          common::compilerBarrier();
      } while (_handle->lock != seq);
      return {};
    }
  };

  class EventSet
  {
    std::array<Event, XPEDITE_PMC_CTRL_CORE_EVENT_MAX> _events;
    int _size;
    int _generation;

    public:

    EventSet(int generation_ = {})
      : _events {}, _size {}, _generation {generation_} {
    }

    void add(Event&& event_) {
      _events[_size++] = std::move(event_);
    }

    void read(uint64_t* buffer_) const noexcept {
      for(int i=0; i<_size; ++i) {
        buffer_[i] = _events[i].read();
      }
    }

    int size() const noexcept {
      return _size;
    }

    int generation() const noexcept {
      return _generation;
    }
  };
}}
