////////////////////////////////////////////////////////////////////////////////////
//
// SamplesLoader loads probe sample data from binary files
//
// Xpedite probes store timing and performance counter data using variable 
// length POD objects. A collection of sample objects is grouped and written
// as a batch. 
//
// The loader iterates through the POD collection,  to extract 
// records in string format for consumption by the profiler
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/persistence/Segment.H>

namespace xpedite { namespace persistence {

  class SegmentLoader
  {
    const CpuInfo* _cpuInfo;
    ProbeInfoMapPtr _probeInfoMap;
    EventsPtr _events;
    TopdownNodesPtr _topdownNodes;
    MetricsPtr _metrics;

    public:

    SegmentLoader()
      : _cpuInfo {}, _probeInfoMap {std::make_shared<ProbeInfoMap>()}, _events {std::make_shared<Events>()},
        _topdownNodes {std::make_shared<TopdownNodes>()}, _metrics {std::make_shared<Metrics>()} {
    }

    std::tuple<const CpuInfo*, ProbeInfoMapPtr, EventsPtr, TopdownNodesPtr, MetricsPtr> data() const noexcept {
      return std::make_tuple(_cpuInfo, _probeInfoMap, _events, _topdownNodes, _metrics);
    }

    void load(const CpuInfoSegment* segment_) {
      auto [info, _] = segment_->data();
      _cpuInfo = info;
    }

    void load(const ProbesSegment* segment_) {
      segment_->forEach([this](const ProbeInfo* info_) {
        _probeInfoMap->add(info_);
      });
    }

    void load(const EventsSegment* segment_) {
      segment_->forEach([this](const EventInfo* info_) {
        _events->emplace_back(info_);
      });
    }

    void load(const TopdownSegment* segment_) {
      segment_->forEach([this](const TopdownNodeInfo* info_) {
        _topdownNodes->emplace_back(info_);
      });
    }

    void load(const MetricsSegment* segment_) {
      segment_->forEach([this](const MetricInfo* info_) {
        _metrics->emplace_back(info_);
      });
    }
  };

}}
