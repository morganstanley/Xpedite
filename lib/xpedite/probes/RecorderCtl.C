////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite Recorder control - Provides logic to setup recorders for a profile session
//
// The class exposes API to select trampolines and corresponding recorders
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/probes/RecorderCtl.H>
#include <xpedite/log/Log.H>

XpediteRecorder activeXpediteRecorder {xpediteExpandAndRecord};

XpediteDataProbeRecorder activeXpediteDataProbeRecorder {xpediteExpandAndRecordWithData};

void* xpediteTrampolinePtr {reinterpret_cast<void*>(xpediteTrampoline)};

void* xpediteDataProbeTrampolinePtr {reinterpret_cast<void*>(xpediteDataProbeTrampoline)};

void* xpediteIdentityTrampolinePtr {reinterpret_cast<void*>(xpediteIdentityTrampoline)};

namespace xpedite { namespace probes {

  RecorderCtl RecorderCtl::_instance;

  RecorderType activeRecorderType {};

  const std::array<std::string, 5> recorderNames { "Trivial", "Expandable", "PMC", "Perf Events", "Logging" };

  const std::string unknown {"Unknown"};

  inline int recorderIndex(RecorderType type_) {
    return static_cast<int>(type_);
  }

  inline const std::string& recorderName(RecorderType type_) {
    unsigned index = recorderIndex(type_);
    if(index < recorderNames.size()) {
      return recorderNames[index];
    }
    return unknown;
  }


  RecorderCtl::RecorderCtl()
    : _recorders {}, _dataRecorders {} {
    _recorders[recorderIndex(RecorderType::TRIVIAL_RECORDER     )] = xpediteRecord;
    _recorders[recorderIndex(RecorderType::EXPANDABLE_RECORDER  )] = xpediteExpandAndRecord;
    _recorders[recorderIndex(RecorderType::PMC_RECORDER         )] = xpediteRecordPmc;
    _recorders[recorderIndex(RecorderType::EVENT_SET_RECORDER   )] = xpediteRecordEventSet;
    _recorders[recorderIndex(RecorderType::lOGGING_RECORDER     )] = xpediteRecordAndLog;

    _dataRecorders[recorderIndex(RecorderType::TRIVIAL_RECORDER     )] = xpediteRecordWithData;
    _dataRecorders[recorderIndex(RecorderType::EXPANDABLE_RECORDER  )] = xpediteExpandAndRecordWithData;
    _dataRecorders[recorderIndex(RecorderType::PMC_RECORDER         )] = xpediteRecordPmcWithData;
    _dataRecorders[recorderIndex(RecorderType::EVENT_SET_RECORDER   )] = xpediteRecordEventSetWithData;
    _dataRecorders[recorderIndex(RecorderType::lOGGING_RECORDER     )] = xpediteRecordWithDataAndLog;
  }

  RecorderType RecorderCtl::activeXpediteRecorderType() noexcept {
    return activeRecorderType;
  }

  bool RecorderCtl::canActivateRecorder(RecorderType type_) noexcept {
    auto index = recorderIndex(type_);
    return static_cast<unsigned>(index) < _recorders.size() && _recorders[index] 
      && static_cast<unsigned>(index) < _dataRecorders.size() && _dataRecorders[index];
  }

  bool RecorderCtl::activateRecorder(RecorderType type_) noexcept {
    bool nonTrivial {recorderIndex(type_) >= recorderIndex(RecorderType::PMC_RECORDER)};
    auto index = recorderIndex(type_);
    if(canActivateRecorder(type_)) {
      activeRecorderType = type_;
      activeXpediteRecorder = _recorders[index];
      activeXpediteDataProbeRecorder = _dataRecorders[index];

      xpediteTrampolinePtr = reinterpret_cast<void*>(trampoline(false, false, nonTrivial));
      xpediteDataProbeTrampolinePtr = reinterpret_cast<void*>(trampoline(true, false, nonTrivial));
      xpediteIdentityTrampolinePtr = reinterpret_cast<void*>(trampoline(false, true, nonTrivial));

      XpediteLogInfo << "Activated " << recorderName(type_) << " recorder" << XpediteLogEnd;
      return true;
    }
    return {};
  }

  Trampoline RecorderCtl::trampoline(bool canStoreData_, bool canSuspendTxn_, bool nonTrivial_) noexcept {
    if(canStoreData_) {
      return nonTrivial_ ? xpediteDataProbeRecorderTrampoline : xpediteDataProbeTrampoline;
    }
    else if(canSuspendTxn_) {
      return nonTrivial_ ? xpediteIdentityRecorderTrampoline : xpediteIdentityTrampoline;
    }
    return nonTrivial_ ? xpediteRecorderTrampoline : xpediteTrampoline;
  }

  Trampoline RecorderCtl::trampoline(bool canStoreData_, bool canSuspendTxn_) noexcept {
    bool nonTrivial {recorderIndex(activeRecorderType) >= recorderIndex(RecorderType::PMC_RECORDER)};
    return trampoline(canStoreData_, canSuspendTxn_, nonTrivial);
  }

}}
