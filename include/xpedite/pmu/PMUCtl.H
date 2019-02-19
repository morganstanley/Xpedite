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

#pragma once

#include <xpedite/util/Allocator.H>
#include <xpedite/perf/PerfEventsCtl.H>
#include <xpedite/pmu/FixedPmcSet.H>
#include <xpedite/util/Tsc.H>
#include <vector>
#include <sys/types.h>

namespace xpedite { namespace perf { namespace test {
  struct Override;
}}}

namespace xpedite { namespace pmu {

  class PmuCtl : public perf::PerfEventsCtl, public util::AlignedObject<XPEDITE_CACHELINE_SIZE>
  {
    struct InertEvents {
      PerfEventSetMap _events;
      uint64_t _generation;
      uint64_t _tsc;

      InertEvents(PerfEventSetMap&& events_, uint64_t generation_)
        : _events {std::move(events_)}, _generation {generation_}, _tsc {RDTSC()}
      {
      }

      InertEvents(const InertEvents&) = delete;
      InertEvents& operator=(const InertEvents&) = delete;
      InertEvents(InertEvents&&) = default;
      InertEvents& operator=(InertEvents&&) = default;
      ~InertEvents() = default;
    };

    std::vector<InertEvents> _inertEventsQueue;
    alignas(XPEDITE_CACHELINE_SIZE) uint8_t _genericPmcCount;

    FixedPmcSet _fixedPmcSet;

    static const uint64_t DEFAULT_QUIESCE_DURATION {5 * 60 * 1000000 * 4};

    static uint64_t _quiesceDuration;

    static PmuCtl* _instance;

    friend struct perf::test::Override;

    public:

    PmuCtl();

    uint8_t genericPmcCount() const noexcept { return _genericPmcCount;                   }
    FixedPmcSet fixedPmcSet() const noexcept { return _fixedPmcSet;                       }
    uint8_t fixedPmcCount()   const noexcept { return _fixedPmcSet.size();                }
    uint8_t pmcCount()        const noexcept { return _genericPmcCount + fixedPmcCount(); }
    bool perfEventsEnabled()  const noexcept { return PerfEventsCtl::isEnabled();         }

    void enableGenericPmc(uint8_t genericPmcCount_) noexcept;
    void disableGenericPmc() noexcept;

    void enableFixedPmc(uint8_t index_) noexcept;
    void disableFixedPmc() noexcept;

    bool enablePerfEvents(const PMUCtlRequest& request_);
    void disablePerfEvents() noexcept;

    bool attachPerfEvents(framework::SamplesBuffer* samplesBuffer_);

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

    void poll();

    static PmuCtl& get() {
      if(!_instance) {
        _instance = new PmuCtl {};
      }
      return *_instance;
    }
  };

  inline PmuCtl& pmuCtl() {
    return PmuCtl::get();
  }

}}
