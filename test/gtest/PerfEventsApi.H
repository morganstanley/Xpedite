///////////////////////////////////////////////////////////////////////////////////////////////
//
// Mock for linux perf api, used to consistency check pmu programming logic.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include "Override.H"
#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/perf/PerfEvent.H>
#include <xpedite/perf/PerfEventAttrSet.H>
#include <xpedite/perf/PerfEventsApi.H>
#include <stdexcept>
#include <map>
#include <functional>

namespace xpedite { namespace perf { namespace test {

  struct PerfEventsApi : public xpedite::perf::PerfEventsApi
  {
    using EventMap = perf_event_mmap_page;

    struct EventState {
      int _fd;
      EventMap _mmap;
      pid_t _tid;
      int _groupFd;
      int _groupSize;
      int _mapCount;
      int _unmapCount;
      int _resetCount;
      int _activationCount;
      int _deactivationCount;
      int _closeCount;

      EventState(pid_t tid_, int groupFd_)
        : _fd {}, _mmap {}, _tid {tid_}, _groupFd {groupFd_}, _groupSize {}, _mapCount {},
          _unmapCount {}, _resetCount {}, _activationCount {}, _deactivationCount {}, _closeCount {} {
      }

      bool isOpen() {
        return _unmapCount == 0 && _deactivationCount == 0 && _closeCount == 0 && _mapCount == 1;
      }

      bool isActive() {
        return isOpen() && _activationCount == 1;
      }

      bool isValid() {
        return (
          (_unmapCount        == 0 || _unmapCount        == 1) &&
          (_resetCount        == 0 || _resetCount        == 1) &&
          (_activationCount   == 0 || _activationCount   == 1) &&
          (_deactivationCount == 0 || _deactivationCount == 1) &&
          (_closeCount        == 0 || _closeCount        == 1)
        );
      }

      bool isLeader() {
        return _groupFd == -1;
      }

      int groupSize() {
        return _groupSize + static_cast<int>(isLeader());
      }
    };

    private:

    std::map<int, EventState> _eventStates;
    std::map<EventMap*, EventState*> _addrSpaces;
    int _nextFd;
    int _inActiveEventsCount;
    Override::Guard _guard;

    public:

    PerfEventsApi()
      : _eventStates {}, _nextFd {}, _inActiveEventsCount {}, _guard {Override::perfEventsApi(this)}{
    }

    int eventsCount() {
      return _eventStates.size();
    }

    int closedEventsCount() {
      return _inActiveEventsCount;
    }

    int openEventsCount() {
      return eventsCount() - closedEventsCount();
    }

    EventState& lookup(int fd_) {
      auto iter = _eventStates.find(fd_);
      if(iter == _eventStates.end()) {
        throw std::runtime_error {"Invariant violation - detected invalid file descriptor"};
      }
      return iter->second;
    }

    EventState& lookup(EventMap* addr_) {
      auto iter = _addrSpaces.find(addr_);
      if(iter == _addrSpaces.end()) {
        throw std::runtime_error {"Invariant violation - detected invalid event map address"};
      }
      return *iter->second;
    }

    EventState& lookup(PerfEvent& event_) {
      return lookup(event_.fd());
    }

    int open(const perf_event_attr*, pid_t tid_, int /*cpu_*/, int groupFd_, unsigned long /*flags_*/) {
      auto fd = _nextFd++;
      auto rc = _eventStates.emplace(std::make_pair(fd, EventState {tid_, groupFd_}));
      if(!rc.second) {
        throw std::runtime_error {"Invariant violation - detected duplicate file descriptor"};
      }

      if(groupFd_ != -1) {
        auto& groupEventState = lookup(groupFd_);
        if(++groupEventState._groupSize > XPEDITE_PMC_CTRL_CORE_EVENT_MAX) {
          throw std::runtime_error {"Invariant violation - exceeded max event group count"};
        }
      }
      return fd;
    }

    EventMap* map(int fd_, size_t) {
      auto& eventState = lookup(fd_);
      if(eventState._mapCount++) {
        throw std::runtime_error {"Invariant violation - detected multiple attempts to map events map"};
      }
      auto rc = _addrSpaces.emplace(std::make_pair(&eventState._mmap, &eventState));
      if(!rc.second) {
        throw std::runtime_error {"Invariant violation - detected duplicate event map address"};
      }
      return &eventState._mmap;
    }

    bool unmap(EventMap* addr_, size_t) {
      auto& eventState = lookup(addr_);
      if(eventState._unmapCount++) {
        throw std::runtime_error {"Invariant violation - detected multiple attempts to unmap events map"};
      }
      return true;
    }

    bool enable(int fd_) {
      auto& eventState = lookup(fd_);
      if(eventState._activationCount++) {
        throw std::runtime_error {"Invariant violation - detected multiple attempts to activate event"};
      }
      return true;
    }

    bool reset(int fd_) {
      auto& eventState = lookup(fd_);
      if(eventState._resetCount++) {
        throw std::runtime_error {"Invariant violation - detected multiple attempts to reset event"};
      }
      return true;
    }

    bool disable(int fd_) {
      auto& eventState = lookup(fd_);
      if(eventState._deactivationCount++) {
        throw std::runtime_error {"Invariant violation - detected multiple attempts to deactivate event"};
      }
      return true;
    }

    bool close(int fd_) {
      auto& eventState = lookup(fd_);
      if(eventState._closeCount++) {
        throw std::runtime_error {"Invariant violation - detected multiple attempts to close event"};
      }
      ++_inActiveEventsCount;
      return true;
    }
  };

}}}
