///////////////////////////////////////////////////////////////////////////////
//
// Segment - a segment in the samples file that stores a type of data
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/persistence/SegmentHeader.H>
#include <xpedite/probes/Sample.H>
#include <xpedite/persistence/CpuInfo.H>
#include <xpedite/persistence/ProbeInfo.H>
#include <xpedite/persistence/EventInfo.H>
#include <vector>
#include <cstring>

namespace xpedite { namespace persistence {

  class Persister;
  class FileHeader;

  class SegmentBase
  {
    protected:

    SegmentHeader _hdr;

    SegmentBase(SegmentHeader::Type type_, timeval time_, unsigned seq_)
      : _hdr {type_, time_, seq_} {
    }

    SegmentBase(SegmentHeader::Type type_, timeval time_, unsigned seq_, uint32_t count_, uint32_t size_)
      : _hdr {type_, time_, seq_, count_, size_} {
    }

    public:

    const SegmentHeader& hdr() const noexcept { return _hdr;        }
    uint32_t size()            const noexcept { return _hdr.size(); }

  } __attribute__((packed));

  template<SegmentHeader::Type type, typename Data>
  class Segment : SegmentBase
  {
    friend class FileHeader;
    friend class Persister;

    Segment(timeval time_, unsigned seq_)
      : SegmentBase {type, time_, seq_} {
    }

    Segment(timeval time_, unsigned seq_, uint32_t count_, uint32_t size_)
      : SegmentBase {type, time_, seq_, count_, size_} {
    }

    template<typename... Args>
    uint32_t add(Args&&... args_) {
      auto* data = new (reinterpret_cast<char*>(this) + _hdr.size()) Data {std::forward<Args>(args_)...};
      _hdr.incrmentCount();
      _hdr.incrmentSize(data->size());
      return data->size();
    }

    const Data* _data() const noexcept {
      return reinterpret_cast<const Data*>(reinterpret_cast<const char*>(this) + _hdr.hdrSize());
    }

    public:

    template<typename Callback>
    void forEach(Callback callback) const {
      const Data* data {_data()};
      for(uint32_t i=0; i<_hdr.count(); ++i) {
        callback(data);
        data = reinterpret_cast<const Data*>(reinterpret_cast<const char*>(data) + data->size());
      }
      assert(reinterpret_cast<const char*>(this)+_hdr.size() == reinterpret_cast<const char*>(data));
    }

    std::tuple<const Data*, unsigned> data() const noexcept {
      return std::make_tuple(_data(), static_cast<unsigned>(_hdr.dataSize()));
    }
  } __attribute__((packed));

  using CpuInfoSegment = Segment<SegmentHeader::Type::CpuInfo, CpuInfo>;
  using ProbesSegment  = Segment<SegmentHeader::Type::Probes, ProbeInfo>;
  using EventsSegment  = Segment<SegmentHeader::Type::Events, EventInfo>;
  using TopdownSegment = Segment<SegmentHeader::Type::TopdownNodes, TopdownNodeInfo>;
  using MetricsSegment = Segment<SegmentHeader::Type::Metrics, MetricInfo>;
  using SamplesSegment = Segment<SegmentHeader::Type::Samples, probes::Sample>;

}}
