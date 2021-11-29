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
#include <xpedite/ux/UxEvent.H>

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

    using UxEvent = ux::UxEvent;

    int _gpEventsCount;
    std::vector<int> _fixedEventIndices;
    std::vector<UxEvent> _events;
    std::vector<std::string> _topdownNodes;
    std::vector<std::string> _metrics;

    public:

    PmuActivationRequest(int gpEventsCount_, std::vector<int> fixedEventIndices_, std::vector<std::string> events_ = {},
        std::vector<std::string> topdownNodes_ = {}, std::vector<std::string> metrics_ = {})
      : _gpEventsCount {gpEventsCount_}, _fixedEventIndices {std::move(fixedEventIndices_)}, _events {},
      _topdownNodes {std::move(topdownNodes_)}, _metrics {std::move(metrics_)} {
      for(auto& event : events_) {
        _events.emplace_back(UxEvent {std::move(event)});
      }
    }

    void execute(Handler& handler_) override {
      handler_.enableGpPMU(_gpEventsCount);
      for(auto fixedEventIndex : _fixedEventIndices) {
        handler_.enableFixedPMU(fixedEventIndex);
      }
      _response.setValue("");
    }

    const char* typeName() const override                { return "PmuActivationRequest";        }
    const std::vector<UxEvent>& events() const           { return _events;                       }
    const std::vector<std::string>& topdownNodes() const { return _topdownNodes;                 }
    const std::vector<std::string>& metrics() const      { return _metrics;                      }
  };

  class PerfEventsActivationRequest : public Request {

    using UxEvent = ux::UxEvent;

    PMUCtlRequest _request;
    std::vector<UxEvent> _events;
    std::vector<std::string> _topdownNodes;
    std::vector<std::string> _metrics;

    public:

    PerfEventsActivationRequest(const PMUCtlRequest& request_, std::vector<std::string> events_ = {},
        std::vector<std::string> topdownNodes_ = {}, std::vector<std::string> metrics_ = {})
      : _request {request_}, _events {}, _topdownNodes {std::move(topdownNodes_)}, _metrics {std::move(metrics_)}{
      for(auto& event : events_) {
        _events.emplace_back(UxEvent {std::move(event)});
      }
    }

    void execute(Handler& handler_) override {
      if(handler_.enablePerfEvents(_request, std::move(_events), std::move(_topdownNodes), std::move(_metrics))) {
        _response.setValue("");
      }
      else {
        _response.setErrors("Failed to enable perf events - check target app stdout for more details.");
      }
    }

    const char* typeName() const override                { return "PerfEventsActivationRequest"; }
    const std::vector<UxEvent>& events() const           { return _events;                       }
    const std::vector<std::string>& topdownNodes() const { return _topdownNodes;                 }
    const std::vector<std::string>& metrics() const      { return _metrics;                      }
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
