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

#pragma once

#include <xpedite/util/Allocator.H>
#include <xpedite/pmu/FixedPmcSet.H>
#include <xpedite/util/Tsc.H>
#include "Event.H"
#include <atomic>

namespace xpedite { namespace pmu {

  class PmuCtl : public util::AlignedObject<XPEDITE_CACHELINE_SIZE>
  {
    volatile std::atomic<int> _generation;

    EventSelect _eventSelect;

    alignas(XPEDITE_CACHELINE_SIZE) uint8_t _genericPmcCount;

    FixedPmcSet _fixedPmcSet;

    static PmuCtl _instance;

    void publishEventSelect(const EventSelect& eventSelect_) noexcept;

    public:

    PmuCtl();

    uint8_t genericPmcCount() const noexcept { return _genericPmcCount;                   }
    FixedPmcSet fixedPmcSet() const noexcept { return _fixedPmcSet;                       }
    uint8_t fixedPmcCount()   const noexcept { return _fixedPmcSet.size();                }
    uint8_t pmcCount()        const noexcept { return _genericPmcCount + fixedPmcCount(); }

    void enableGenericPmc(uint8_t genericPmcCount_) noexcept;
    void disableGenericPmc() noexcept;

    void enableFixedPmc(uint8_t index_) noexcept;
    void disableFixedPmc() noexcept;

    bool enable(const PMUCtlRequest& request_);
    void disable() noexcept;

    int snapEventSelect(EventSelect& eventSelect_) const noexcept;

    static EventSet buildEventSet(const EventSelect& eventSelect_, int generation_) noexcept;

    void readPmc(uint64_t* buffer_) noexcept {
      int i {};
      for (; i < _genericPmcCount; ++i) {
        buffer_[i] = RDPMC(i);
      }

      if(_fixedPmcSet.isEnabled<pmu::FixedPmcSet::INST_RETIRED_ANY>()) {
        buffer_[i++] = RDPMC(0x40000000);
      }

      if(_fixedPmcSet.isEnabled<pmu::FixedPmcSet::CPU_CLK_UNHALTED_CORE>()) {
        buffer_[i++] = RDPMC(0x40000001);
      }
      
      if(_fixedPmcSet.isEnabled<pmu::FixedPmcSet::CPU_CLK_UNHALTED_REF>()) {
        buffer_[i++] = RDPMC(0x40000002);
      }
    }

    static PmuCtl& get() {
      return _instance;
    }
  };

  inline PmuCtl& pmuCtl() {
    return PmuCtl::get();
  }

}}
