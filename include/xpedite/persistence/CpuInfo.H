///////////////////////////////////////////////////////////////////////////////
//
// CpuInfo - Info about cpu model and configuration
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/persistence/SegmentHeader.H>
#include <string>
#include <sstream>

namespace xpedite::persistence {

  template<SegmentHeader::Type, typename> class Segment;

  class CpuInfo
  {
    template<SegmentHeader::Type, typename> friend class Segment;
    uint64_t _frequency;
    uint64_t _frequencyKhz;
    double _cyclesPerUsec;
    uint32_t  _cpuIdOffset;
    uint32_t  _cpuIdSize;
    uint32_t _size;
    char _data[0];

    CpuInfo(std::string_view cpuId_, uint64_t frequency_)
      : _frequency {frequency_}, _frequencyKhz {frequency_/1000}, _cyclesPerUsec {_frequencyKhz / 1000.0},
        _cpuIdOffset {}, _size {} {
      _cpuIdSize = cpuId_.size();
      memcpy(_data+_size, cpuId_.data(), cpuId_.size());
      _size += cpuId_.size();
    }

    CpuInfo (const CpuInfo&)            = delete;
    CpuInfo& operator=(const CpuInfo&)  = delete;
    CpuInfo (CpuInfo&&)                 = delete;
    CpuInfo& operator=(CpuInfo&&)       = delete;

    public:

    uint64_t frequency()       const noexcept { return _frequency;              }
    uint64_t frequencyKhz()    const noexcept { return _frequencyKhz;           }
    double cyclesPerUsec()     const noexcept { return _cyclesPerUsec;          }
    const char* cpuId()        const noexcept { return _data + _cpuIdOffset;    }
    uint32_t cpuIdSize()       const noexcept { return _cpuIdSize;              }
    size_t size()              const noexcept { return sizeof(CpuInfo) + _size; }

    double convertCyclesToTime(long cycles_) const noexcept {
      return cycles_ / _cyclesPerUsec; 
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "CpuInfo {id : " << cpuId() << ", frequency : " << frequency() << "}";
      return stream.str();
    }
  } __attribute__((packed));
}

namespace std {
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::persistence::CpuInfo& cpuInfo_) {
    stream_ << cpuInfo_.toString();
    return stream_;
  }
}
