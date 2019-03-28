///////////////////////////////////////////////////////////////////////////////////////////////
//
// ProfileInfo - a type used to configure parameters of a profiling session.
//
// The profile info contains the following
//   1. Set of probes to be enabled for a profiling session
//   2. A list of pmc counters to be programmed
//   3. Max capacity of files used for storing sample data
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/probes/ProbeKey.H>
#include <xpedite/probes/RecorderCtl.H>
#include <xpedite/pmu/EventSet.h>
#include <vector>
#include <string>
#include <algorithm>

namespace xpedite { namespace framework {

  class ProfileInfo
  {
    using ProbeKey = probes::ProbeKey;
    std::vector<ProbeKey> _probes;
    PMUCtlRequest _pmuRequest;
    uint64_t _samplesDataCapacity;

    XpediteRecorder _recorder;
    XpediteDataProbeRecorder _dataProbeRecorder;

    public:

    ProfileInfo(std::vector<std::string> probes_, const PMUCtlRequest& pmuRequest_, uint64_t samplesDataCapacity_ = {})
      : _probes {}, _pmuRequest {pmuRequest_}, _samplesDataCapacity {samplesDataCapacity_},
        _recorder {}, _dataProbeRecorder {} {
      _probes.reserve(probes_.size());
      std::for_each(probes_.begin(), probes_.end(), [this](std::string& name_) {
        _probes.emplace_back(ProbeKey {std::move(name_)});
      });
    }

    ProfileInfo(std::vector<ProbeKey> probes_, const PMUCtlRequest& pmuRequest_, uint64_t samplesDataCapacity_ = {})
      : _probes {std::move(probes_)}, _pmuRequest {pmuRequest_}, _samplesDataCapacity {samplesDataCapacity_},
        _recorder {}, _dataProbeRecorder {} {
    }

    const std::vector<ProbeKey>& probes() const {
      return _probes;
    }

    void addProbe(std::string name_) {
      _probes.emplace_back(ProbeKey {std::move(name_)});
    }

    const PMUCtlRequest& pmuRequest() const {
      return _pmuRequest;
    }

    uint64_t samplesDataCapacity() const {
      return _samplesDataCapacity;
    }

    void overrideRecorder(XpediteRecorder recorder_, XpediteDataProbeRecorder dataProbeRecorder_) noexcept {
      _recorder = recorder_;
      _dataProbeRecorder = dataProbeRecorder_;
    }

    XpediteRecorder recorder() const noexcept {
      return _recorder;
    }

    XpediteDataProbeRecorder dataProbeRecorder() const noexcept {
      return _dataProbeRecorder;
    }
  };

}}
