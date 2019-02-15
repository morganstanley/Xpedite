///////////////////////////////////////////////////////////////////////////////
//
// Logic to program pmu events using linux perf events api
//
// PerfEvent - Abstraction for reading h/w pmc using perf events api
//
// A Perf event owns and manages scope/lifetime, of the file descriptor and 
// mapped memory provided by the linux perf api
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/platform/Builtins.H>
#include <xpedite/util/Tsc.H>
#include <algorithm>
#include <linux/perf_event.h>

namespace xpedite { namespace perf {

  class PerfEvent
  {
    static const int INVALID_FD;
    static perf_event_mmap_page* INVALID_ADDR;

    int _fd;
    perf_event_mmap_page* _handle;
    pid_t _tid;

    public:

    PerfEvent() noexcept
      : _fd {INVALID_FD}, _handle {INVALID_ADDR}, _tid {} {
    }

    PerfEvent(perf_event_attr attr_, int gid_, pid_t tid_) noexcept;
    PerfEvent(const PerfEvent&) = delete;
    PerfEvent& operator=(const PerfEvent&) = delete;

    PerfEvent(PerfEvent&& other_) noexcept
      : _fd {other_._fd}, _handle {other_._handle}, _tid {other_._tid} {
      other_._fd = INVALID_FD;
      other_._handle = {INVALID_ADDR};
      other_._tid = {};
    }

    PerfEvent& operator=(PerfEvent&& other_) noexcept {
      std::swap(_fd, other_._fd);
      std::swap(_handle, other_._handle);
      std::swap(_tid, other_._tid);
      return *this;
    }

    ~PerfEvent() noexcept;

    int fd() const noexcept {
      return _fd;
    }

    perf_event_mmap_page* buffer() const noexcept {
      return _handle;
    }

    int tid() const noexcept {
      return _tid;
    }

    explicit operator bool() const noexcept {
      return (_fd != INVALID_FD) && (_handle != INVALID_ADDR);
    }

    uint64_t read() const noexcept {
      uint32_t seq;
      uint64_t pmc {};
      do {
          seq = _handle->lock;
          common::compilerBarrier();
          pmc = _handle->offset;
          if (auto idx = _handle->index) {
            pmc += RDPMC(idx - 1);
          }
          common::compilerBarrier();
      } while (_handle->lock != seq);
      return pmc;
    }
  };

}}
