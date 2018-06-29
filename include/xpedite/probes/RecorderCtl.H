////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite Recorder control - Provides logic to setup recorders for a profile session
//
// The class exposes API to enable / reset generic and fixed pmu events
//
// Enabling event, automatically sets the appropriate recorders
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/probes/FixedPmcSet.H>
#include <xpedite/probes/Recorders.H>

using XpediteRecorder = void (*)(const void*, uint64_t);
using XpediteDataProbeRecorder = void (*)(const void*, uint64_t, __uint128_t);

extern XpediteRecorder activeXpediteRecorder;
extern XpediteDataProbeRecorder activeXpediteDataProbeRecorder;

namespace xpedite { namespace probes {

  class RecorderCtl
  {
    using Recorders = std::array<XpediteRecorder, 16>;
    using DataProbeRecorders = std::array<XpediteDataProbeRecorder, 16>;

    Recorders _recorders;
    DataProbeRecorders _dataRecorders;
    uint8_t _genericPmcCount;
    FixedPmcSet _fixedPmcSet;

    static RecorderCtl _instance;

    RecorderCtl();

    public:

    uint8_t genericPmcCount() const noexcept { return _genericPmcCount;                   }
    FixedPmcSet fixedPmcSet() const noexcept { return _fixedPmcSet;                       }
    uint8_t fixedPmcCount()   const noexcept { return _fixedPmcSet.size();                }
    uint8_t pmcCount()        const noexcept { return _genericPmcCount + fixedPmcCount(); }


    void enableGenericPmc(uint8_t genericPmcCount_) noexcept;
    void resetGenericPmc() noexcept;

    void enableFixedPmc(uint8_t index_) noexcept;
    void resetFixedPmc() noexcept;

    int activeRecorderIndex() noexcept;
    int activeDataProbeRecorderIndex() noexcept;
    bool canActivateRecorder(int index_) noexcept;
    bool activateRecorder(int index_) noexcept;

    static RecorderCtl& get() {
      return _instance;
    }
  };

  inline RecorderCtl& recorderCtl() {
    return RecorderCtl::get();
  }

}}
