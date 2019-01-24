///////////////////////////////////////////////////////////////////////////////
//
// PerfEventsCtl - Logic to program and collect perf events.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <xpedite/perf/PerfEventSet.H>
#include <mutex>
#include <memory>
#include <map>
#include <tuple>
#include <sys/types.h>

namespace xpedite { namespace framework {
  class SamplesBuffer;
}}

namespace xpedite { namespace perf {

  class PerfEventsCtl
  {
    protected:

    using PerfEventSetPtr = std::unique_ptr<PerfEventSet>;

    using PerfEventSetMap = std::map<pid_t, PerfEventSetPtr>;

    uint64_t generation() const noexcept {
      return _generation;
    }

    const PerfEventSetMap& activeEvents() const noexcept {
      return _activeEvents;
    }

    public:

    PerfEventsCtl();

    bool isEnabled()  const noexcept {
      return _isEnabled;
    }

    bool enable(const PerfEventAttrSet& eventAttrs_, PerfEventSetMap& inertEvents_);

    PerfEventSetMap disable() noexcept;

    bool attachTo(framework::SamplesBuffer* samplesBuffer_, PerfEventSetPtr& inertEventSetPtr_);

    private:

    PerfEventSetPtr attachUnsafe(framework::SamplesBuffer* samplesBuffer_, PerfEventSet&& perfEventSet_);

    void publishEventAttrs(const PerfEventAttrSet& eventAttrs_) noexcept;

    std::tuple<uint64_t, PerfEventAttrSet> snapEventAttrs() const noexcept;

    PerfEventAttrSet _activeEventAttrs;

    PerfEventSetMap _activeEvents;

    mutable std::mutex _mutex;

    uint64_t _generation;

    bool _isEnabled;
  };

}}
