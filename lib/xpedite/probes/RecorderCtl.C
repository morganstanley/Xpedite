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

#include <xpedite/probes/RecorderCtl.H>
#include <xpedite/log/Log.H>

XpediteRecorder activeXpediteRecorder {xpediteExpandAndRecord};

XpediteDataProbeRecorder activeXpediteDataProbeRecorder {xpediteExpandAndRecordWithData};

xpedite::probes::Trampoline xpediteTrampolinePtr {xpediteTrampoline};

xpedite::probes::Trampoline xpediteDataProbeTrampolinePtr {xpediteDataProbeTrampoline};

xpedite::probes::Trampoline xpediteIdentityTrampolinePtr {xpediteIdentityTrampoline};

namespace xpedite { namespace probes {

  RecorderCtl RecorderCtl::_instance;

  RecorderCtl::RecorderCtl()
    : _recorders {
        xpediteExpandAndRecord,
        xpediteRecord,
        xpediteRecordPmc,
        xpediteRecordAndLog
      }, 
      _dataRecorders {
        xpediteExpandAndRecordWithData,
        xpediteRecordWithData,
        xpediteRecordPmcWithData,
        xpediteRecordWithDataAndLog
      },
      _genericPmcCount {},
      _fixedPmcSet {} {
  }

  int RecorderCtl::activeRecorderIndex() noexcept {
    for(int i=0; i<static_cast<int>(_recorders.size()) && _recorders[i]; ++i) {
      if(_recorders[i] == activeXpediteRecorder) {
        return i;
      }
    }
    return -1;
  }

  int RecorderCtl::activeDataProbeRecorderIndex() noexcept {
    for(int i=0; i<static_cast<int>(_dataRecorders.size()) && _dataRecorders[i]; ++i) {
      if(_dataRecorders[i] == activeXpediteDataProbeRecorder) {
        return i;
      }
    }
    return -1;
  }

  bool RecorderCtl::canActivateRecorder(int index_) noexcept {
    return static_cast<unsigned>(index_) < _recorders.size() && _recorders[index_] 
      && static_cast<unsigned>(index_) < _dataRecorders.size() && _dataRecorders[index_];
  }

  bool RecorderCtl::activateRecorder(int index_, bool nonTrivial_) noexcept {
    if(canActivateRecorder(index_)) {
      activeXpediteRecorder = _recorders[index_];
      activeXpediteDataProbeRecorder = _dataRecorders[index_];

      xpediteTrampolinePtr = trampoline(false, false, nonTrivial_);
      xpediteDataProbeTrampolinePtr = trampoline(true, false, nonTrivial_);
      xpediteIdentityTrampolinePtr = trampoline(false, true, nonTrivial_);

      XpediteLogInfo << "Activated recorder at index " << index_ << XpediteLogEnd;
      return true;
    }
    return {};
  }

  void RecorderCtl::enableGenericPmc(uint8_t genericPmcCount_) noexcept {
    if(pmcCount() == 0) {
      activateRecorder(2, true);
    }
    _genericPmcCount = genericPmcCount_;
  }

  void RecorderCtl::resetGenericPmc() noexcept {
    if(_genericPmcCount) {
      _genericPmcCount = 0;
      if(pmcCount() == 0) {
        activateRecorder(0, false);
      }
    }
  }

  void RecorderCtl::enableFixedPmc(uint8_t index_) noexcept {
    if(pmcCount() == 0) {
      activateRecorder(2, true);
    }
    _fixedPmcSet.enable(index_);
  }

  void RecorderCtl::resetFixedPmc() noexcept {
    if(_fixedPmcSet.size()) {
      _fixedPmcSet.reset();
      if(pmcCount() == 0) {
        activateRecorder(0, false);
      }
    }
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
    return trampoline(canStoreData_, canSuspendTxn_, pmcCount()>0);
  }

}}
