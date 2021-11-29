//////////////////////////////////////////////////////////////////////////////////////////////
//
// Collector - collects probe data captured by application threads
//
// usage
// beginSamplesCollection() - prepares the collector for sample collection
// poll()                   - polls and copies new samples to free space in samples buffers
// endSamplesCollection()   - flushes samples and ends collection
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "StorageMgr.H"
#include <xpedite/framework/Persister.H>
#include <string>
#include <tuple>

namespace xpedite { namespace probes {
  class Sample;
}}

namespace xpedite { namespace framework {

  class SamplesBuffer;

  class Collector
  {
    public:

    Collector(std::string fileNamePattern_, uint64_t samplesDataCapacity_)
      : _storageMgr {samplesDataCapacity_}, _persister {}, _fileNamePattern {std::move(fileNamePattern_)},
        _isCollecting {}, _capacityBreached {} {
    }

    ~Collector() {
      if(isCollecting()) {
        endSamplesCollection();
      }
    }

    bool isCollecting() const noexcept {
      return _isCollecting;
    }

    bool beginSamplesCollection();
    bool endSamplesCollection();
    void poll(bool flush_ = false);

    private:

    void persistSamples(int fd_, const probes::Sample* begin_, const probes::Sample* end_);
    std::tuple<int, int, int> collectSamples(SamplesBuffer* buffer_);
    std::tuple<int, int> flush(SamplesBuffer* buffer_);

    StorageMgr _storageMgr;
    Persister _persister;
    std::string _fileNamePattern;
    bool _isCollecting;
    bool _capacityBreached;
  };

}}
