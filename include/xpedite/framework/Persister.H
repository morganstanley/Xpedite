///////////////////////////////////////////////////////////////////////////////
//
// Methods to persist probe timing and pmc data to filesystem
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/probes/Sample.H>
#include <xpedite/framework/CallSiteInfo.H>
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
    uint64_t signature() const {return _signature;}
    bool isValid() const noexcept {
      return _signature == XPEDITE_SEGMENT_HDR_SIG;
    }

  } __attribute__((packed));

  class FileHeader
  {
    uint64_t _signature;
    uint64_t _version;
    timeval  _time;
    uint64_t _tscHz;
    uint32_t _pmcCount;
    uint32_t _callSiteCount;
    CallSiteInfo _callSites[0];

    public:

    static constexpr uint64_t XPEDITE_VERSION {0x0200};
    static constexpr uint64_t XPEDITE_FILE_HDR_SIG {0xC01DC01DC0FFEEEE};

    static size_t callSiteSize(uint64_t callSiteCount_) {
      return sizeof(CallSiteInfo) * callSiteCount_;
    }

    static size_t capacity(uint64_t callSiteCount_) {
      return sizeof(FileHeader) + callSiteSize(callSiteCount_);
    }

    FileHeader(const std::vector<CallSiteInfo>& callSites_, timeval time_, uint64_t tscHz_, uint32_t pmcCount_)
      : _signature {XPEDITE_FILE_HDR_SIG}, _version {XPEDITE_VERSION}, _time (time_),
        _tscHz {tscHz_}, _pmcCount {pmcCount_}, _callSiteCount {static_cast<uint32_t>(callSites_.size())} {
      memcpy(_callSites, &callSites_.front(), callSiteSize(callSites_.size()));
    }

    unsigned int getSize() const noexcept {
      return _callSiteCount*sizeof(CallSiteInfo);
    }

    bool isValid() const noexcept {
      return _signature == XPEDITE_FILE_HDR_SIG && _version == XPEDITE_VERSION;
    }

    timeval time()      const noexcept { return _time;     }
    uint64_t tscHz()    const noexcept { return _tscHz;    }
    uint32_t pmcCount() const noexcept { return _pmcCount; }

    const SegmentHeader* segmentHeader() const noexcept {
      return reinterpret_cast<const SegmentHeader*>(reinterpret_cast<const char*>(this + 1) + callSiteSize(_callSiteCount));
    }

    std::tuple<const CallSiteInfo*, uint32_t> callSites() const noexcept {
      return std::make_tuple(&_callSites[0], _callSiteCount);
    }
  } __attribute__((packed));

  void persistHeader(int fd_);
  void persistData(int fd_, const probes::Sample* begin_, const probes::Sample* end_);

}}
