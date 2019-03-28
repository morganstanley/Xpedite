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

xpedite::probes::Trampoline xpediteTrampolinePtr {xpediteTrampoline};

xpedite::probes::Trampoline xpediteDataProbeTrampolinePtr {xpediteDataProbeTrampoline};

xpedite::probes::Trampoline xpediteIdentityTrampolinePtr {xpediteIdentityTrampoline};

namespace xpedite { namespace probes {

  RecorderCtl* RecorderCtl::_instance {};

  RecorderType activeRecorderType {RecorderType::EXPANDABLE_RECORDER};

  inline int recorderIndex(RecorderType type_) {
    return static_cast<int>(type_);
  }

  inline const char* recorderName(RecorderType type_) {
    switch(type_) {
      case (RecorderType::TRIVIAL_RECORDER):
        return "Trivial";
      case (RecorderType::EXPANDABLE_RECORDER):
        return "Expandable";
      case (RecorderType::PMC_RECORDER):
        return "PMC";
      case (RecorderType::PERF_EVENTS_RECORDER):
        return "Perf Events";
      case (RecorderType::LOGGING_RECORDER):
        return "Logging";
      case (RecorderType::CUSTOM_RECORDER):
        return "Custom";
    }
    return "Unknown";
  }


  RecorderCtl::RecorderCtl()
    : _recorders {}, _dataRecorders {} {
    _recorders[recorderIndex(RecorderType::TRIVIAL_RECORDER     )] = xpediteRecord;
    _recorders[recorderIndex(RecorderType::EXPANDABLE_RECORDER  )] = xpediteExpandAndRecord;
    _recorders[recorderIndex(RecorderType::PMC_RECORDER         )] = xpediteRecordPmc;
    _recorders[recorderIndex(RecorderType::PERF_EVENTS_RECORDER )] = xpediteRecordPerfEvents;
    _recorders[recorderIndex(RecorderType::LOGGING_RECORDER     )] = xpediteRecordAndLog;
    _recorders[recorderIndex(RecorderType::CUSTOM_RECORDER      )] = xpediteExpandAndRecord;

    _dataRecorders[recorderIndex(RecorderType::TRIVIAL_RECORDER     )] = xpediteRecordWithData;
    _dataRecorders[recorderIndex(RecorderType::EXPANDABLE_RECORDER  )] = xpediteExpandAndRecordWithData;
    _dataRecorders[recorderIndex(RecorderType::PMC_RECORDER         )] = xpediteRecordPmcWithData;
    _dataRecorders[recorderIndex(RecorderType::PERF_EVENTS_RECORDER )] = xpediteRecordPerfEventsWithData;
    _dataRecorders[recorderIndex(RecorderType::LOGGING_RECORDER     )] = xpediteRecordWithDataAndLog;
    _dataRecorders[recorderIndex(RecorderType::CUSTOM_RECORDER      )] = xpediteExpandAndRecordWithData;
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
    if(canActivateRecorder(type_)) {
      activeRecorderType = type_;
      auto index = recorderIndex(type_);
      activeXpediteRecorder = _recorders[index];
      activeXpediteDataProbeRecorder = _dataRecorders[index];

      bool nonTrivial {recorderIndex(type_) >= recorderIndex(RecorderType::PMC_RECORDER)};
      xpediteTrampolinePtr = trampoline(false, false, nonTrivial);
      xpediteDataProbeTrampolinePtr = trampoline(true, false, nonTrivial);
      xpediteIdentityTrampolinePtr = trampoline(false, true, nonTrivial);

      XpediteLogInfo << "Activated " << recorderName(type_) << " recorder" << XpediteLogEnd;
      return true;
    }
    return {};
  }

  bool RecorderCtl::activateRecorder(XpediteRecorder recorder_, XpediteDataProbeRecorder dataProbeRecorder_) noexcept {
    if(!recorder_ || !dataProbeRecorder_) {
      XpediteLogError << "Failed to active custom recorder - detected null recorder callback(s)" << XpediteLogEnd;
      return {};
    }
    activeRecorderType = RecorderType::CUSTOM_RECORDER;
    activeXpediteRecorder = recorder_;
    activeXpediteDataProbeRecorder = dataProbeRecorder_;
    xpediteTrampolinePtr = trampoline(false, false, true);
    xpediteDataProbeTrampolinePtr = trampoline(true, false, true);
    xpediteIdentityTrampolinePtr = trampoline(false, true, true);
    XpediteLogInfo << "Activated custom recorder [" << std::hex << recorder_ << " | " << dataProbeRecorder_ 
      << std::dec << "]" << XpediteLogEnd;
    return true;
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
