////////////////////////////////////////////////////////////////////////////////////////////
//
// Profile - Provides logic to track state of probes and pmc configurations
//
// The profile object keeps track of, changes made by a profiler during a profile session.
// The state is resotred to original process state, at the end of profiling.
//   1. Stores the list of activated probes and de-activates at end of session
//   2. Resets Fixed and General purpose pmc configurations at end of session
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/probes/RecorderCtl.H>
#include <xpedite/probes/ProbeCtl.H>
#include <xpedite/pmu/EventSet.h>
#include <xpedite/pmu/Formatter.h>
#include <xpedite/log/Log.H>
#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/probes/ProbeKey.H>
#include <set>
#include <string>

namespace xpedite { namespace framework {

  class Profile
  {
    std::set<probes::ProbeKey> _activeProbes;

    public:

    void activateProbe(const probes::ProbeKey& key_) {
      XpediteLogInfo << "xpedite enabling probe | name - " << key_.name()
        << " | file - " << key_.file() << " | line = " << key_.line() << " |" << XpediteLogEnd;
      const auto* probeName = key_.name().empty() ? nullptr : key_.name().c_str();
      probes::probeCtl(probes::Command::ENABLE, key_.file().c_str(), key_.line(), probeName);
      _activeProbes.emplace(key_);
    }

    void deactivateProbe(const probes::ProbeKey& key_) {
      _activeProbes.erase(key_);
      XpediteLogInfo << "xpedite disabling probe | name - " << key_.name() 
        << " | file - " << key_.file() << " | line = " << key_.line() << " |" << XpediteLogEnd;
      const auto* probeName = key_.name().empty() ? nullptr : key_.name().c_str();
      probes::probeCtl(probes::Command::DISABLE, key_.file().c_str(), key_.line(), probeName);
    }

    void enableGpPMU(int count_) {
      XpediteLogInfo << "xpedite enabling collection for " << count_ << " general purpose PMU counters" << XpediteLogEnd;
      pmu::pmuCtl().enableGenericPmc(count_);
    }

    void disableGpPMU() {
      if(pmu::pmuCtl().genericPmcCount()) {
        XpediteLogInfo << "xpedite disabling collection for general purpose PMU counters" << XpediteLogEnd;
        pmu::pmuCtl().disableGenericPmc();
      }
    }

    void enableFixedPMU(uint8_t index_) {
      pmu::pmuCtl().enableFixedPmc(index_);
      XpediteLogInfo << "xpedite enabled fixed PMU counter (" << static_cast<int>(index_) << ") -> " 
        << pmu::pmuCtl().fixedPmcSet().toString() << XpediteLogEnd;
    }

    void disableFixedPMU() {
      if(pmu::pmuCtl().fixedPmcCount()) {
        XpediteLogInfo << "xpedite disabling collection of " << static_cast<int>(pmu::pmuCtl().fixedPmcCount())
          << " fixed architectural PMU counters" << XpediteLogEnd;
        pmu::pmuCtl().disableFixedPmc();
      }
    }

    bool enablePerfEvents(const PMUCtlRequest& request_) {
      char buffer[4096];
      pmuRequestToString(&request_, buffer, sizeof(buffer));
      XpediteLogInfo << "xpedite Rx PMU for request \n" 
        << "\n----------------------------------------------------------------------------------------------------------"
        << buffer 
        << "\n----------------------------------------------------------------------------------------------------------"
        << XpediteLogEnd;
      return pmu::pmuCtl().enablePerfEvents(request_);
    }

    void disablePerfEvents() {
      XpediteLogInfo << "xpedite disabling perf events" << XpediteLogEnd;
      return pmu::pmuCtl().disablePerfEvents();
    }

    void disablePMU() {
      if(pmu::pmuCtl().perfEventsEnabled()) {
        disablePerfEvents();
      }
      else {
        disableGpPMU();
        disableFixedPMU();
      }
    }

    void start() noexcept {
    }

    void stop() noexcept {
      XpediteLogInfo << "xpedite disabling " << _activeProbes.size() << " probes" << XpediteLogEnd;
      for(auto& probe : _activeProbes) {
        XpediteLogInfo << "xpedite disabling probe | name - " << probe.name()
          << " | file - " << probe.file() << " | line = " << probe.line() << " |" << XpediteLogEnd;
        probes::probeCtl(probes::Command::DISABLE, probe.file().c_str(), probe.line(), probe.name() != "" ? probe.name().c_str() : nullptr);
      }

      disablePMU();

      if(probes::recorderCtl().activeXpediteRecorderType() == probes::RecorderType::CUSTOM_RECORDER) {
        probes::recorderCtl().activateRecorder(probes::RecorderType::EXPANDABLE_RECORDER);
      }
    }
  };

}}
