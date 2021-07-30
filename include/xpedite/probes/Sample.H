///////////////////////////////////////////////////////////////////////////////
//
// Classes to store timing, pmc and probe data 
//
// Sample - a variable length POD object to store probe sample data
//
// SamplesHeader - used for batching a collection of samples
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/util/Tsc.H>
#include <xpedite/platform/Builtins.H>
#include <xpedite/probes/Recorders.H>
#include <xpedite/pmu/FixedPmcSet.H>
#include <cstdint>
#include <tuple>
#include <sstream>
#include <cassert>

namespace xpedite { namespace common {
  template<typename T, size_t Size> struct Buffer;
}}

namespace xpedite { namespace probes {

  struct Probe;

  class Sample
  {
    using Data = __uint128_t;

    using AliasingData = __attribute__((__may_alias__)) __uint128_t;

    static constexpr uint64_t FLAG_DATA {1UL << 62};
    static constexpr uint64_t FLAG_PMC  {1UL << 63};
    static constexpr uint64_t FLAGS     {FLAG_PMC | FLAG_DATA};
    static constexpr uint64_t TSC_MASK  {~FLAGS};

    uint64_t _tsc;
    const void* _returnSite;
    uint64_t _data[0];

    Sample() = default;

    Sample(const void* returnSite_, uint64_t tsc_)
      : _tsc{tsc_}, _returnSite {returnSite_} {
    }

    Sample(const void* returnSite_, uint64_t tsc_, Data data_)
      : Sample{returnSite_, tsc_ | FLAG_DATA} {
      *reinterpret_cast<AliasingData*>(_data) = data_;
    }

    Sample(const void* returnSite_, uint64_t tsc_, bool /*collectPmc*/)
      : Sample {returnSite_, tsc_ | FLAG_PMC} {
      _data[0] = pmu::pmuCtl().pmcCount();
      pmu::pmuCtl().readPmc(_data + 1);
    }

    Sample(const void* returnSite_, uint64_t tsc_, Data data_, bool /*collectPmc*/)
      : Sample {returnSite_, tsc_ | FLAG_PMC, data_} {
      _data[2] = pmu::pmuCtl().pmcCount();
      pmu::pmuCtl().readPmc(_data + 3);
    }

    Sample(const void* returnSite_, uint64_t tsc_, const perf::PerfEventSet* eventSet_)
      : Sample {returnSite_, tsc_ | FLAG_PMC} {
      _data[0] = eventSet_->size();
      eventSet_->read(_data + 1);
    }

    Sample(const void* returnSite_, uint64_t tsc_, Data data_, const perf::PerfEventSet* eventSet_)
      : Sample {returnSite_, tsc_ | FLAG_PMC, data_} {
      _data[2] = eventSet_->size();
      eventSet_->read(_data + 3);
    }

    Sample(const Sample&)            = delete;
    Sample& operator=(const Sample&) = delete;
    Sample(Sample&&)                 = delete;
    Sample& operator=(Sample&&)      = delete;

    template<typename T, size_t Size> friend struct common::Buffer;

    friend void XPEDITE_CALLBACK ::xpediteExpandAndRecord(const void*, uint64_t);
    friend void XPEDITE_CALLBACK ::xpediteRecordAndLog(const void*, uint64_t);
    friend void XPEDITE_CALLBACK ::xpediteRecord(const void*, uint64_t);
    friend void XPEDITE_CALLBACK ::xpediteRecordPmc(const void*, uint64_t);
    friend void XPEDITE_CALLBACK ::xpediteRecordPerfEvents(const void*, uint64_t);

    friend void XPEDITE_CALLBACK ::xpediteExpandAndRecordWithData(const void*, uint64_t, __uint128_t);
    friend void XPEDITE_CALLBACK ::xpediteRecordWithDataAndLog(const void*, uint64_t, __uint128_t);
    friend void XPEDITE_CALLBACK ::xpediteRecordWithData(const void*, uint64_t, __uint128_t);
    friend void XPEDITE_CALLBACK ::xpediteRecordPmcWithData(const void*, uint64_t, __uint128_t);
    friend void XPEDITE_CALLBACK ::xpediteRecordPerfEventsWithData(const void*, uint64_t, __uint128_t);

    public:

    inline unsigned size() const noexcept {
      /*******************************************************************
       * pmcCount() may refer to memory past the end of Sample object
       * However, Samples can only created in SamplesBuffer, which
       * provides a guard space to afford this kind of access
       *******************************************************************/
      return sizeof(Sample) + sizeof(uint64_t) * (hasData()*2 + hasPmc()*(1 + pmcCount()));
    }

    inline const void* returnSite() const noexcept {
      return _returnSite;
    }

    inline uint64_t tsc() const noexcept {
      return _tsc & TSC_MASK;
    }

    inline bool hasData() const noexcept {
      return _tsc & FLAG_DATA;
    }

    inline bool hasPmc() const noexcept {
      return _tsc & FLAG_PMC;
    }

    inline uint64_t pmcCount() const noexcept {
      return _data[2*hasData()] & 0xF;
    }

    inline std::tuple<uint64_t, uint64_t> data() const noexcept {
      return std::make_tuple(_data[0], _data[1]);
    }

    inline std::tuple<const uint64_t*, int> pmc() const noexcept {
      return std::make_tuple(&_data[1 + 2*hasData()], pmcCount());
    }

    inline uint64_t pmc(int index_) const noexcept {
      const uint64_t* v; int c;
      std::tie(v, c) = pmc();
			assert(index_ < c);
      return v[index_];
    }

    inline static constexpr unsigned maxSize() noexcept {
      // user data            - 2 * sizeof(uint64_t)
      // number of counters   - 1 * sizeof(uint64_t)
      // pmc counter          - 8 * sizeof(uint64_t)
      // fixed counter        - 3 * sizeof(uint64_t)
      return sizeof(Sample) + sizeof(uint64_t) * 14;
    }

    inline Sample* next() noexcept {
      return reinterpret_cast<Sample*>(reinterpret_cast<char*>(this) + size());
    }

    inline const Sample* next() const noexcept {
      return const_cast<Sample*>(this)->next();
    }

    std::string toString() const {
      std::ostringstream os;
      os << "Sample[" << std::hex << _returnSite << "]" << std::dec << "{" << 
        "tsc - " << tsc() << " | " << "size - " << size();
        if(hasData()) {
          os << " | data [" << std::get<0>(data()) << "," << std::get<1>(data()) << "]";
        }
        if(hasPmc()) {
          const uint64_t* v;
          int c;
          std::tie(v, c) = pmc();
          os << " | pmc [" << v[0];
          for(int i=1; i<c; ++i) {
            os << ", " << v[i];
          }
          os << "]";
        }
        os << "}";
      return os.str();
    }
  };

}}
