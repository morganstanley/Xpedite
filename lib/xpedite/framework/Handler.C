////////////////////////////////////////////////////////////////////////////////////////
//
// Handler to lookup and execute commands from profiler
//
// Handler provides the following functionality.
//   1. Api to register commands and callbacks
//   2. Tokenizer to extract command and arguments from frames
//   3. Command mapping and execution of callbacks
//   4. Support for heartbeats, starting and stopping of profiling sessions
// 
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////

#include "Handler.H"
#include <xpedite/util/Tsc.H>
#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/probes/ProbeList.H>
#include <xpedite/log/Log.H>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>

namespace xpedite { namespace framework {

  std::string Handler::ping() const noexcept {
    return "hello";
  }

  uint64_t Handler::tscHz() const noexcept {
    return util::estimateTscHz();
  }

  std::string Handler::beginProfile(std::string samplesFilePattern_, MilliSeconds pollInterval_, uint64_t samplesDataCapacity_) {
    if(isProfileActive()) {
      auto errMsg = "xpedite failed to begin profile - session already active";
      XpediteLogError << errMsg << XpediteLogEnd;
      return errMsg;
    }

    if(samplesFilePattern_.empty()) {
      auto errMsg = "xpedite failed to begin profile - samples file pattern not specified";
      XpediteLogError << errMsg << XpediteLogEnd;
      return errMsg;
    }

    if(!_pollInterval.count()) {
      auto errMsg = "xpedite failed to begin profile - poll interval must be a valid number of milli seconds";
      XpediteLogError << errMsg << XpediteLogEnd;
      return errMsg;
    }

    _pollInterval = pollInterval_;
    XpediteLogInfo << "xpedite starting collecter - sample file - " << samplesFilePattern_
       << " | poll interval - every " << _pollInterval.count() << " milli seconds | samplesDataCapacity - "
       << samplesDataCapacity_ << " bytes" << XpediteLogEnd;
    _collector.reset(new Collector {std::move(samplesFilePattern_), samplesDataCapacity_});

    if(!_collector->beginSamplesCollection()) {
      std::ostringstream stream;
      stream << "xpedite - failed to initialize collector - check application stdout for more details";
      auto errMsg = stream.str();
      XpediteLogError << errMsg << XpediteLogEnd;
      _collector.reset();
      return errMsg;
    }
    _profile.start();
    return {};
  }

  std::string Handler::endProfile() {
    _profile.stop();
    if(!_collector) {
      return "profiling not active - can't end something that's not started";
    }
    _collector->endSamplesCollection();
    _collector.reset();
    return {};
  }

  std::string Handler::listProbes() {
    std::ostringstream stream;
    log::logProbes(stream, probes::probeList());
    return stream.str();
  }

  void Handler::activateProbe(const probes::ProbeKey& key_) {
    _profile.activateProbe(key_);
  }

  void Handler::deactivateProbe(const probes::ProbeKey& key_) {
    _profile.deactivateProbe(key_);
  }

  void Handler::enableGpPMU(int count_) {
    _profile.enableGpPMU(count_);
  }

  void Handler::enableFixedPMU(uint8_t index_) {
    _profile.enableFixedPMU(index_);
  }

  bool Handler::enablePerfEvents(const PMUCtlRequest& request_) {
    return _profile.enablePerfEvents(request_);
  }

  void Handler::disablePMU() {
    _profile.disablePMU();
  }

  Handler::Handler()
    : _pollInterval {10} /*10 milli second*/ {
  }

  void Handler::shutdown() {
    if(isProfileActive()) {
      endProfile();
    }
    _collector.reset();
  }

  void Handler::poll() {
    if(_collector) {
      _collector->poll();
    }
    pmu::pmuCtl().poll();
  }

}}
