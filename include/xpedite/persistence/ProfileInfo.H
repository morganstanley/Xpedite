///////////////////////////////////////////////////////////////////////////////
//
// ProfileInfo
// 
// Info about a profile
// Includes cpu info, probes, pmu events and topdown metrics
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/ux/UxProbe.H>
#include <xpedite/persistence/CpuInfo.H>
#include <xpedite/persistence/EventInfo.H>
#include <xpedite/persistence/ProbeHandle.H>

#include <string>
#include <sstream>

namespace xpedite::persistence {

  class ProfileInfo
  {
    const CpuInfo* _cpuInfo;
    ProbeHandlesPtr _probes;
    EventsPtr _events;
    TopdownNodesPtr _topdownNodes;
    MetricsPtr _metrics;

    public:

    ProfileInfo(const CpuInfo* cpuInfo_, ProbeHandlesPtr probes_, EventsPtr events_,
      TopdownNodesPtr topdownNodes_, MetricsPtr metrics_)
      : _cpuInfo {std::move(cpuInfo_)}, _probes {std::move(probes_)}, _events {std::move(events_)},
        _topdownNodes {std::move(topdownNodes_)}, _metrics {std::move(metrics_)} {
    }

    const CpuInfo* cpuInfo()             const noexcept { return _cpuInfo;      }
    const ProbeHandlesPtr probes()       const noexcept { return _probes;       }
    const EventsPtr events()             const noexcept { return _events;       }
    const TopdownNodesPtr topdownNodes() const noexcept { return _topdownNodes; }
    const MetricsPtr metrics()           const noexcept { return _metrics;      }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Profile info:" << std::endl;
      stream << "\t\t" << *_cpuInfo << std::endl;
      stream << "\t\t" << *_probes << std::endl;
      stream << "\t\t" << *_events << std::endl;
      stream << "\t\t" << *_topdownNodes << std::endl;
      stream << "\t\t" << *_metrics << std::endl;
      return stream.str();
    }
  };
}

namespace std {
  inline std::iostream& operator<<(std::iostream& stream_, const xpedite::persistence::ProfileInfo& info_) {
    stream_ << info_.toString();
    return stream_;
  }
}
