///////////////////////////////////////////////////////////////////////////////
//
// Methods to persist probe timing and pmc data to filesystem
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/probes/Sample.H>
#include <xpedite/framework/ProbeInfo.H>
#include <vector>
#include <cstring>

namespace xpedite { namespace framework {

  class SegmentHeader
  {
    static constexpr uint64_t XPEDITE_SEGMENT_HDR_SIG {0x5CA1AB1E887A57EFUL};

    uint64_t _signature;
    timeval  _time;
    uint32_t _size;
    uint32_t _seq;

    public:

    SegmentHeader(timeval time_, unsigned size_, unsigned seq_)
      : _signature {XPEDITE_SEGMENT_HDR_SIG}, _time (time_), _size {size_}, _seq {seq_} {
    }

    std::tuple<const probes::Sample*, unsigned> samples() const noexcept {
      return std::make_tuple(reinterpret_cast<const probes::Sample*>(this + 1), static_cast<unsigned>(_size));
    }

    timeval time()  const noexcept { return _time; }
    uint32_t size() const noexcept { return _size; }
    uint32_t seq()  const noexcept { return _seq;  }

  } __attribute__((packed));

  class FileHeader
  {
    uint64_t _signature;
    uint64_t _version;
    timeval  _time;
    uint64_t _tscHz;
    uint32_t _pmcCount;
    uint32_t _probeCount;
    uint32_t _probeInfoBufSize;
    ProbeInfo _probes[0];

    public:

    static constexpr uint64_t XPEDITE_MIN_COMPATIBLE_VERSION {0x0200};
    static constexpr uint64_t XPEDITE_VERSION {0x0210};
    static constexpr uint64_t XPEDITE_FILE_HDR_SIG {0xC01DC01DC0FFEEEE};

    size_t size() const noexcept {
      return sizeof(FileHeader) + _probeInfoBufSize;
    }

    FileHeader(timeval time_, uint64_t tscHz_, uint32_t pmcCount_)
      : _signature {XPEDITE_FILE_HDR_SIG}, _version {XPEDITE_VERSION}, _time (time_),
        _tscHz {tscHz_}, _pmcCount {pmcCount_}, _probeCount {}, _probeInfoBufSize {} {
    }

    FileHeader(const FileHeader&)             = delete;
    FileHeader& operator=(const FileHeader&)  = delete;
    FileHeader(FileHeader&&)                  = delete;
    FileHeader& operator=(FileHeader&&)       = delete;

    bool isValid() const noexcept {
      return _signature == XPEDITE_FILE_HDR_SIG && _version >= XPEDITE_MIN_COMPATIBLE_VERSION && _version <= XPEDITE_VERSION;
    }

    timeval time()           const noexcept { return _time;       }
    uint64_t tscHz()         const noexcept { return _tscHz;      }
    uint32_t pmcCount()      const noexcept { return _pmcCount;   }
    uint32_t probeCount()    const noexcept { return _probeCount; }

    const SegmentHeader* segmentHeader() const noexcept {
      if(_version < XPEDITE_VERSION) {
        auto bufSize = sizeof(FileHeader) - 4 + _probeCount * 16;
        return reinterpret_cast<const SegmentHeader*>(reinterpret_cast<const char*>(this) + bufSize);
      }
      return reinterpret_cast<const SegmentHeader*>(reinterpret_cast<const char*>(this + 1) + _probeInfoBufSize);
    }

    void add(const void* callSite_, probes::CallSiteAttr attr_, uint32_t id_, Name probeName_,
        Name fileName_, Name function_, uint32_t lineNo_) {
      ++_probeCount;
      auto* probeInfo = new (reinterpret_cast<char*>(this + 1) + _probeInfoBufSize) ProbeInfo {
        callSite_, attr_, id_, probeName_, fileName_, function_, lineNo_
      };
      _probeInfoBufSize += probeInfo->size();
    }

    template<typename T>
    void forEachCallSiteInfo(T t) const {
      if(_version < XPEDITE_VERSION) {
        return;
      }
      const ProbeInfo* probeInfo {_probes};
      for(uint32_t i=0; i<_probeCount; ++i) {
        t(probeInfo);
        probeInfo = reinterpret_cast<const ProbeInfo*>(reinterpret_cast<const char*>(probeInfo) + probeInfo->size());
      }
      assert(reinterpret_cast<const char*>(this)+size() == reinterpret_cast<const char*>(probeInfo));
    }

  } __attribute__((packed));

  class Persister
  {
    FileHeader* _hdr;
    std::vector<char> _buffer;

    void resizeBuffer(size_t objSize_);

    size_t freeSize() const noexcept {
      return _buffer.size() - _hdr->size();
    }

    public:

    Persister();

    void persistHeader(int fd_);
    void persistData(int fd_, const probes::Sample* begin_, const probes::Sample* end_);
  };

}}
