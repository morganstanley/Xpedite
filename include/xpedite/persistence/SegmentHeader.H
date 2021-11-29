///////////////////////////////////////////////////////////////////////////////
//
// Segment header - Header for a segment in the xpedite samples file
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/probes/Sample.H>
#include <vector>
#include <cstring>

namespace xpedite { namespace persistence {

  struct SegmentHeader
  {
    enum class Type : uint32_t {
      CpuInfo,
      Probes,
      Events,
      TopdownNodes,
      Metrics,
      Samples,
      Unknown,
    };

    SegmentHeader(Type type_, timeval time_, unsigned seq_)
      : _signature {XPEDITE_SEGMENT_HDR_SIG}, _type {type_}, _time {time_}, _count {},
        _hdrSize {sizeof(SegmentHeader)}, _dataSize {}, _seq {seq_} {
    }

    SegmentHeader(Type type_, timeval time_, unsigned seq_, uint32_t count_, uint32_t size_)
      : _signature {XPEDITE_SEGMENT_HDR_SIG}, _type {type_}, _time {time_}, _count {count_},
        _hdrSize {sizeof(SegmentHeader)}, _dataSize {size_}, _seq {seq_} {
    }

    Type type() const noexcept {
      return isVersionCurrent() ? _type : Type::Probes;
    }

    uint32_t hdrSize() const noexcept {
      constexpr auto oldHdrsize = sizeof(_signature) + sizeof(_time) + sizeof(_hdrSize) + sizeof(_seq);
      return isVersionCurrent() ? _hdrSize : oldHdrsize;
    }

    uint32_t dataSize() const noexcept {
      if(isVersionCurrent()) {
        return _dataSize;
      } else {
        auto sizeOffset = sizeof(_signature) + sizeof(_time);
        return *(reinterpret_cast<const uint32_t*>(reinterpret_cast<const char*>(this) + sizeOffset));
      }
    }


    timeval time()          const noexcept { return _time;                                 }
    uint32_t count()        const noexcept { return _count;                                }
    uint32_t size()         const noexcept { return hdrSize() + dataSize();                }
    uint32_t seq()          const noexcept { return _seq;                                  }
    bool isVersionCurrent() const noexcept { return _signature == XPEDITE_SEGMENT_HDR_SIG; }

    void incrmentCount(uint32_t count_ = 1) noexcept { _count += count_;   }
    void incrmentSize(uint32_t size_)       noexcept { _dataSize += size_; }

    private:

    static constexpr uint64_t XPEDITE_SEGMENT_HDR_SIG {0x5CA1AB1E007A57EFUL};

    const uint64_t _signature;
    const Type _type;
    const timeval  _time;
    uint32_t _count;
    const uint32_t _hdrSize;
    uint32_t _dataSize;
    uint32_t _seq;

  } __attribute__((packed));

}}
