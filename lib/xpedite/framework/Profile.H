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
#include <set>
#include <string>

namespace xpedite { namespace framework {

  class ProbeKey
  {
    std::string _name;
    std::string _file;
    uint32_t _line;

    friend bool operator<(const ProbeKey& lhs_, const ProbeKey& rhs_) noexcept;

    public:

    template<typename F, typename N>
    ProbeKey(N&& name_, F&& file_, uint32_t line_)
      : _name {std::forward<N>(name_)}, _file {std::forward<F>(file_)}, _line {line_} {
    }

    const std::string name()  const noexcept { return _name; }
    const std::string& file() const noexcept { return _file; }
    uint32_t line()           const noexcept { return _line; }
  };

  inline bool operator<(const ProbeKey& lhs_, const ProbeKey& rhs_) noexcept {
    if(lhs_._file == rhs_._file) {
      return lhs_._line == rhs_._line ? lhs_._name < rhs_._name : lhs_._line < rhs_._line;
    }
    return lhs_._file < rhs_._file;
  }

  class Profile
  {
    std::set<ProbeKey> _activeProbes;

    public:

    void enableProbe(const std::string& name_, const std::string& file_, uint32_t line_) {
      XpediteLogInfo << "xpedite enabling probe | name - " << name_
        << " | file - " << file_ << " | line = " << line_ << " |" << XpediteLogEnd;
      probes::probeCtl(probes::Command::ENABLE, file_.c_str(), line_, name_ != "" ? name_.c_str() : nullptr);
      _activeProbes.emplace(name_, file_, line_);
    }

    void disableProbe(const std::string& name_, const std::string& file_, uint32_t line_) {
      _activeProbes.erase({name_, file_, line_});
      XpediteLogInfo << "xpedite disabling probe | name - " << name_ 
        << " | file - " << file_ << " | line = " << line_ << " |" << XpediteLogEnd;
      probes::probeCtl(probes::Command::DISABLE, file_.c_str(), line_, name_ != "" ? name_.c_str() : nullptr);
    }

    void enableGpPMC(int pmcCount_) {
      XpediteLogInfo << "xpedite enabling collection for " << pmcCount_ << " general purpose PMU counters" << XpediteLogEnd;
      pmu::pmuCtl().enableGenericPmc(pmcCount_);
    }

    void disableGpPMC() {
      if(pmu::pmuCtl().genericPmcCount()) {
        XpediteLogInfo << "xpedite disabling collection for general purpose PMU counters" << XpediteLogEnd;
        pmu::pmuCtl().disableGenericPmc();
      }
    }

    void enableFixedPMC(uint8_t index_) {
      pmu::pmuCtl().enableFixedPmc(index_);
      XpediteLogInfo << "xpedite enabled fixed PMU counter (" << static_cast<int>(index_) << ") -> " 
        << pmu::pmuCtl().fixedPmcSet().toString() << XpediteLogEnd;
    }

    void disableFixedPMC() {
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

    void start() noexcept {
    }

    void stop() noexcept {
      XpediteLogInfo << "xpedite disabling " << _activeProbes.size() << " probes" << XpediteLogEnd;
      for(auto& probe : _activeProbes) {
        XpediteLogInfo << "xpedite disabling probe | name - " << probe.name()
          << " | file - " << probe.file() << " | line = " << probe.line() << " |" << XpediteLogEnd;
        probes::probeCtl(probes::Command::DISABLE, probe.file().c_str(), probe.line(), probe.name() != "" ? probe.name().c_str() : nullptr);
      }

      if(pmu::pmuCtl().perfEventsEnabled()) {
        disablePerfEvents();
      }
      else {
        disableGpPMC();
        disableFixedPMC();
      }
    }
  };

}}
