///////////////////////////////////////////////////////////////////////////////
//
// PMUCtl - Logic to program and collect core, fixed and offcore performance
// counters
//
// Supports programming and collection of pmu events using
// 1. RDPMC with events programmed out of band by xpedite kernel module
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
#include <sys/types.h>

namespace xpedite { namespace pmu {

  class PmuCtl : public util::AlignedObject<XPEDITE_CACHELINE_SIZE>
  {
    alignas(XPEDITE_CACHELINE_SIZE) uint8_t _genericPmcCount;

    FixedPmcSet _fixedPmcSet;

    static PmuCtl _instance;

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

    void disable() noexcept;

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
