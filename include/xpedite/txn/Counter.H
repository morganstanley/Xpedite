///////////////////////////////////////////////////////////////////////////////
//
// Counter - stores the timestamp and performance counters for a sample from a 
// probe
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/probes/Sample.H>
#include <xpedite/persistence/ProbeHandle.H>

#include <sstream>

namespace xpedite { namespace txn {

  class Counter
  {
    using Sample = probes::Sample;
    using ProbeHandle = persistence::ProbeHandle;

    size_t _threadId;
    const Sample* _sample;
    const ProbeHandle* _probe;

    public:

    Counter(size_t threadId_, const Sample* sample_, const ProbeHandle* probe_)
      : _threadId {threadId_}, _sample {sample_}, _probe {probe_} {
    }

    size_t threadId()          const noexcept { return _threadId;          }
    const Sample* sample()     const noexcept { return _sample;            }
    const ProbeHandle* probe() const noexcept { return _probe;             }
    uint64_t tsc()             const noexcept { return _sample->tsc();     }
    bool hasData()             const noexcept { return _sample->hasData(); }
    bool hasPmc()              const noexcept { return _sample->hasPmc();  }

    uint64_t pmcCount() const noexcept {
      return hasPmc() ? _sample->pmcCount() : 0;
    }

    std::vector<uint64_t> pmcs() const noexcept {
      return {};
    }

    std::tuple<uint64_t, uint64_t> data() const noexcept {
      return hasData() ? _sample->data() : std::tuple<uint64_t, uint64_t> {};
    }

    std::tuple<const uint64_t*, int> pmc() const noexcept {
      return hasPmc() ? _sample->pmc() : std::tuple<const uint64_t*, int> {};
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Counter {name : " << _probe->name() << ", thread : " << threadId()
        << ", tsc : " << tsc() << "}";
      return stream.str();
    }
  };

}}
