///////////////////////////////////////////////////////////////////////////////
//
// Methods to persist timing and pmc data to filesystem
// The records can be encoded in one these three formats (text, csv, binary) 
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/persistence/Persister.H>
#include <xpedite/persistence/ProbeInfo.H>
#include <xpedite/persistence/EventInfo.H>
#include <xpedite/probes/Config.H>
#include <xpedite/probes/ProbeList.H>
#include <xpedite/util/Util.H>
#include <xpedite/util/Tsc.H>
#include <xpedite/pmu/PMUCtl.H>
#include <sys/time.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

namespace xpedite { namespace persistence {

  void Persister::resizeBuffer(size_t objSize_) {
    auto size = _buffer.size();
    while(size < _hdr->size() + objSize_) {
      size *= 2;
    }
    _buffer.resize(size);
    _hdr = reinterpret_cast<FileHeader*>(_buffer.data());
  }

  Persister::Persister(const std::vector<ux::UxEvent>& events_, const std::vector<std::string>& topdownNodes_,
      const std::vector<std::string>& metrics_)
    : _hdr {}, _buffer {}, _nextSegmentIndex {} {
    auto tscHz = util::estimateTscHz();
    timeval  time;
    gettimeofday(&time, nullptr);
    _buffer.resize(2 * 1024 * 1024);
    _hdr = new (_buffer.data()) FileHeader {time, tscHz, pmu::pmuCtl().pmcCount()};

    auto* cpuInfoSegment = _hdr->addSegment<SegmentHeader::Type::CpuInfo, CpuInfo>(time, ++_nextSegmentIndex);
    std::string cpuIdStr {"UnKnown"};
    std::string_view cpuId {cpuIdStr.c_str(), cpuIdStr.size()+1};
    auto objSize =  sizeof(CpuInfo) + cpuId.size();
    if(freeSize() < objSize) {
      resizeBuffer(objSize);
      cpuInfoSegment = _hdr->currentSegment<SegmentHeader::Type::CpuInfo, CpuInfo>();
    }
    cpuInfoSegment->add(cpuId, tscHz);
    _hdr->finalize(cpuInfoSegment);

    if(probes::probeList().size() > 0) {
      auto* probeSegment = _hdr->addSegment<SegmentHeader::Type::Probes, ProbeInfo>(time, ++_nextSegmentIndex);
      for(auto& probe : probes::probeList()) {
        std::string_view probeName    {probe.name(), static_cast<uint32_t>(strlen(probe.name()))+1};
        std::string_view fileName     {probe.file(), static_cast<uint32_t>(strlen(probe.file()))+1};
        std::string_view functionName {probe.func(), static_cast<uint32_t>(strlen(probe.func()))+1};
        auto objSize =  sizeof(ProbeInfo) + probeName.size() + fileName.size() + functionName.size();
        if(freeSize() < objSize) {
          resizeBuffer(objSize);
          probeSegment = _hdr->currentSegment<SegmentHeader::Type::Probes, ProbeInfo>();
        }
        probeSegment->add(
          probe.recorderReturnSite(), probe.attr(), probe.id(), probeName, fileName, functionName, probe.line()
        );
      }
      _hdr->finalize(probeSegment);
    }

    if(!events_.empty()) {
      auto* eventSegment = _hdr->addSegment<SegmentHeader::Type::Events, EventInfo>(time, ++_nextSegmentIndex);
      for(auto& event : events_) {
        std::string_view eventName {event.name().c_str(), event.name().size()+1};
        auto objSize =  sizeof(EventInfo) + eventName.size();
        if(freeSize() < objSize) {
          resizeBuffer(objSize);
          eventSegment = _hdr->currentSegment<SegmentHeader::Type::Events, EventInfo>();
        }
        eventSegment->add(eventName, event.user(), event.kernel());
      }
      _hdr->finalize(eventSegment);
    }

    if(!topdownNodes_.empty()) {
      auto* topdownSegment = _hdr->addSegment<SegmentHeader::Type::TopdownNodes, TopdownNodeInfo>(time, ++_nextSegmentIndex);
      for(auto& topdownNode : topdownNodes_) {
        std::string_view name {topdownNode.c_str(), topdownNode.size()+1};
        auto objSize =  sizeof(TopdownNodeInfo) + name.size();
        if(freeSize() < objSize) {
          resizeBuffer(objSize);
          topdownSegment = _hdr->currentSegment<SegmentHeader::Type::TopdownNodes, TopdownNodeInfo>();
        }
        topdownSegment->add(name);
      }
      _hdr->finalize(topdownSegment);
    }

    if(!metrics_.empty()) {
      auto* metricSegment = _hdr->addSegment<SegmentHeader::Type::Metrics, MetricInfo>(time, ++_nextSegmentIndex);
      for(auto& metric : metrics_) {
        std::string_view name {metric.c_str(), metric.size()+1};
        auto objSize =  sizeof(MetricInfo) + name.size();
        if(freeSize() < objSize) {
          resizeBuffer(objSize);
          metricSegment = _hdr->currentSegment<SegmentHeader::Type::Metrics, MetricInfo>();
        }
        metricSegment->add(name);
      }
      _hdr->finalize(metricSegment);
    }
  }

  void Persister::persistHeader(int fd_) {
    write(fd_, _buffer.data(), _hdr->size());
    XpediteLogInfo << "persisted file header with " << _hdr->metaSegmentCount() << " meta segments | capacity "
      << _hdr->size() << " bytes" << XpediteLogEnd;
  }

  void Persister::persistData(int fd_, const probes::Sample* begin_, const probes::Sample* end_) {

    if(!begin_ || begin_ == end_) {
      return;
    }
    uint64_t ccstart {RDTSC()};
    timeval  time;
    gettimeofday(&time, nullptr);
    unsigned size = reinterpret_cast<const char*>(end_) - reinterpret_cast<const char*>(begin_);

    using SamplesSegment = Segment<SegmentHeader::Type::Samples, probes::Sample>;
    SamplesSegment samplesSegment {time, ++_nextSegmentIndex, 0, size}; 
    write(fd_, &samplesSegment, sizeof(samplesSegment));
    write(fd_, begin_, size);
    if(probes::config().verbose()) {
      XpediteLogInfo << "persisted segment " << size << " bytes in " << RDTSC() - ccstart << " cycles" << XpediteLogEnd;
    }
  }

}}
