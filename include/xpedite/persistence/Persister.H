///////////////////////////////////////////////////////////////////////////////
//
// Methods to persist probe timing and pmc data to filesystem
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/ux/UxEvent.H>
#include <xpedite/probes/Sample.H>
#include <xpedite/persistence/Segment.H>
#include <vector>
#include <cstring>

namespace xpedite { namespace persistence {

  class FileHeader
  {
    uint64_t _signature;
    uint64_t _version;
    timeval  _time;
    uint64_t _tscHz;
    uint32_t _pmcCount;
    uint32_t _metaSegmentCount;
    uint32_t _metaSegmentSize;
    char _metaSegments[0];

    public:

    static constexpr uint64_t XPEDITE_MIN_COMPATIBLE_VERSION {0x0200};
    static constexpr uint64_t XPEDITE_VERSION {0x0210};
    static constexpr uint64_t XPEDITE_FILE_HDR_SIG {0xC01DC01DC0FFEEEE};

    using SamplesSegment = Segment<SegmentHeader::Type::Samples, probes::Sample>;

    size_t size() const noexcept {
      return sizeof(FileHeader) + _metaSegmentSize;
    }

    FileHeader(timeval time_, uint64_t tscHz_, uint32_t pmcCount_)
      : _signature {XPEDITE_FILE_HDR_SIG}, _version {XPEDITE_VERSION}, _time (time_),
        _tscHz {tscHz_}, _pmcCount {pmcCount_}, _metaSegmentCount {}, _metaSegmentSize {} {
    }

    FileHeader(const FileHeader&)             = delete;
    FileHeader& operator=(const FileHeader&)  = delete;
    FileHeader(FileHeader&&)                  = delete;
    FileHeader& operator=(FileHeader&&)       = delete;

    bool isValid() const noexcept {
      return _signature == XPEDITE_FILE_HDR_SIG && _version >= XPEDITE_MIN_COMPATIBLE_VERSION && _version <= XPEDITE_VERSION;
    }

    timeval time()              const noexcept { return _time;             }
    uint64_t tscHz()            const noexcept { return _tscHz;            }
    uint32_t pmcCount()         const noexcept { return _pmcCount;         }
    uint32_t metaSegmentCount() const noexcept { return _metaSegmentCount; }
    uint32_t metaSegmentSize()  const noexcept { return _metaSegmentSize;  }

    const SamplesSegment* samples() const noexcept {
      if(_version < XPEDITE_VERSION) {
        auto bufSize = sizeof(FileHeader) - 4 + _metaSegmentCount * 16;
        return reinterpret_cast<const SamplesSegment*>(reinterpret_cast<const char*>(this) + bufSize);
      }
      return reinterpret_cast<const SamplesSegment*>(reinterpret_cast<const char*>(this + 1) + _metaSegmentSize);
    }

    template<SegmentHeader::Type type, typename Data>
    Segment<type, Data>* addSegment(timeval time_, unsigned seq_) {
      return new(reinterpret_cast<char*>(this + 1) + _metaSegmentSize) Segment<type, Data> {time_, seq_};
    }

    template<SegmentHeader::Type type, typename Data>
    Segment<type, Data>* currentSegment() {
      return reinterpret_cast<Segment<type, Data>*>(reinterpret_cast<char*>(this + 1) + _metaSegmentSize);
    }

    template<SegmentHeader::Type type, typename Data> 
    void finalize(Segment<type, Data>* segment_) {
      _metaSegmentSize += segment_->hdr().size();
      ++_metaSegmentCount;
    }

    template<typename Loader>
    void forEachMetaSegment(Loader& loader_) const {
      if(_version < XPEDITE_VERSION) {
        return;
      }
      const auto* segment {reinterpret_cast<const SegmentBase*>(reinterpret_cast<const char*>(this + 1))};
      for(uint32_t i=0; i<_metaSegmentCount; ++i) {
        switch(segment->hdr().type()) {
          case SegmentHeader::Type::CpuInfo:
            loader_.load(reinterpret_cast<const CpuInfoSegment*>(segment));
            break;
          case SegmentHeader::Type::Probes:
            loader_.load(reinterpret_cast<const ProbesSegment*>(segment));
          break; 
          case SegmentHeader::Type::Events:
            loader_.load(reinterpret_cast<const EventsSegment*>(segment));
          break; 
          case SegmentHeader::Type::TopdownNodes:
            loader_.load(reinterpret_cast<const TopdownSegment*>(segment));
          break; 
          case SegmentHeader::Type::Metrics:
            loader_.load(reinterpret_cast<const MetricsSegment*>(segment));
          break; 
          default:
          break;
        }
        segment = {reinterpret_cast<const SegmentBase*>(reinterpret_cast<const char*>(segment) + segment->hdr().size())};
      }
      assert(reinterpret_cast<const char*>(this + 1)+_metaSegmentSize == reinterpret_cast<const char*>(segment));
    }

  } __attribute__((packed));

  class Persister
  {
    FileHeader* _hdr;
    std::vector<char> _buffer;
    unsigned _nextSegmentIndex;


    void resizeBuffer(size_t objSize_);

    size_t freeSize() const noexcept {
      return _buffer.size() - _hdr->size();
    }

    public:

    Persister(const std::vector<ux::UxEvent>& events_, const std::vector<std::string>& topdownNodes_,
        const std::vector<std::string>& metrics_);

    void persistHeader(int fd_);
    void persistData(int fd_, const probes::Sample* begin_, const probes::Sample* end_);
  };

}}
