////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite Recorder control - Provides logic to setup recorders for a profile session
//
// The class exposes API to select trampolines and corresponding recorders
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/probes/CallSite.H>
#include <xpedite/probes/Recorders.H>

using XpediteRecorder = void (*)(const void*, uint64_t);
using XpediteDataProbeRecorder = void (*)(const void*, uint64_t, __uint128_t);

extern XpediteRecorder activeXpediteRecorder;
extern XpediteDataProbeRecorder activeXpediteDataProbeRecorder;

namespace xpedite { namespace probes {

  namespace test {
    struct ProbeTest;
  }

  enum class RecorderType
  {
    TRIVIAL_RECORDER,
    EXPANDABLE_RECORDER,
    PMC_RECORDER,
    lOGGING_RECORDER
  };

  class RecorderCtl
  {
    using Recorders = std::array<XpediteRecorder, 16>;
    using DataProbeRecorders = std::array<XpediteDataProbeRecorder, 16>;

    friend test::ProbeTest;

    Recorders _recorders;
    DataProbeRecorders _dataRecorders;

    static RecorderCtl _instance;

    RecorderCtl();

    public:

    RecorderType activeXpediteRecorderType() noexcept;

    bool canActivateRecorder(RecorderType type_) noexcept;
    bool activateRecorder(RecorderType type_) noexcept;

    Trampoline trampoline(bool canStoreData_, bool canSuspendTxn_) noexcept;

    Trampoline trampoline(bool canStoreData_, bool canSuspendTxn_, bool nonTrivial_) noexcept;

    static RecorderCtl& get() {
      return _instance;
    }
  };

  inline RecorderCtl& recorderCtl() {
    return RecorderCtl::get();
  }

}}
