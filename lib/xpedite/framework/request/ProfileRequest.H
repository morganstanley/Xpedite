//////////////////////////////////////////////////////////////////////////////////////////////
//
// ProfileRequest - Group of request types to activate/deactivate
//  1. profiling session
//  2. PMU counters programmed using the kernel module
//  3. Perf events programmed in process context
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Request.H"
#include <xpedite/pmu/EventSet.h>
#include <xpedite/probes/RecorderCtl.H>

namespace xpedite { namespace framework { namespace request {

  class ProfileActivationRequest : public Request {

    std::string _samplesFilePattern;
    MilliSeconds _pollInterval;
    uint64_t _samplesDataCapacity;

    XpediteRecorder _recorder;
    XpediteDataProbeRecorder _dataProbeRecorder;

    public:

    ProfileActivationRequest(std::string samplesFilePattern_, MilliSeconds pollInterval_, uint64_t samplesDataCapacity_)
      : _samplesFilePattern {std::move(samplesFilePattern_)}, _pollInterval {pollInterval_},
        _samplesDataCapacity {samplesDataCapacity_}, _recorder {}, _dataProbeRecorder {} {
    }

    void overrideRecorder(XpediteRecorder recorder_, XpediteDataProbeRecorder dataProbeRecorder_) noexcept {
      _recorder = recorder_;
      _dataProbeRecorder = dataProbeRecorder_;
    }

    void execute(Handler& handler_) override {
      if(_recorder || _dataProbeRecorder) {
        if(!probes::recorderCtl().activateRecorder(_recorder, _dataProbeRecorder)) {
          _response.setErrors("Failed to activate custom recorder");
          return;
        }
      }

      auto rc = handler_.beginProfile(_samplesFilePattern, _pollInterval, _samplesDataCapacity);
      if(rc.empty()) {
        _response.setValue("");
      }
      else {
        _response.setErrors(rc);
      }
    }

    const char* typeName() const override {
      return "ProfileActivationRequest";
    }
  };

  struct ProfileDeactivationRequest : public Request {

    void execute(Handler& handler_) override {
      auto rc = handler_.endProfile();
      if(rc.empty()) {
        _response.setValue("");
      }
      else {
        _response.setErrors(rc);
      }
    }

    const char* typeName() const override {
      return "ProfileDeactivationRequest";
    }
  };

  class PmuActivationRequest : public Request {
    int _gpEventsCount;
    std::vector<int> _fixedEventIndices;

    public:

    PmuActivationRequest(int gpEventsCount_, std::vector<int> fixedEventIndices_)
      : _gpEventsCount {gpEventsCount_}, _fixedEventIndices {std::move(fixedEventIndices_)} {
    }

    void execute(Handler& handler_) override {
      handler_.enableGpPMU(_gpEventsCount);
      for(auto fixedEventIndex : _fixedEventIndices) {
        handler_.enableFixedPMU(fixedEventIndex);
      }
      _response.setValue("");
    }

    const char* typeName() const override {
      return "PmuActivationRequest";
    }
  };

  class PerfEventsActivationRequest : public Request {

    PMUCtlRequest _request;

    public:

    PerfEventsActivationRequest(const PMUCtlRequest& request_)
      : _request {request_} {
    }

    void execute(Handler& handler_) override {
      if(handler_.enablePerfEvents(_request)) {
        _response.setValue("");
      }
      else {
        _response.setErrors("Failed to enable perf events - check target app stdout for more details.");
      }
    }

    const char* typeName() const override {
      return "PerfEventsActivationRequest";
    }
  };

  struct PmuDeactivationRequest : public Request {
    void execute(Handler& handler_) override {
      handler_.disablePMU();
      _response.setValue("");
    }

    const char* typeName() const override {
      return "PmuDeactivationRequest";
    }
  };

}}}
