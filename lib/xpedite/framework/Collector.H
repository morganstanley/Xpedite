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
#include <string>
#include <tuple>
#include <chrono>
#include <thread>

namespace xpedite { namespace probes {
  class Sample;
}}

namespace xpedite { namespace framework {

  class SamplesBuffer;

  class Collector
  {
    public:

    Collector(std::string fileNamePattern_, uint64_t samplesDataCapacity_)
      : _storageMgr {samplesDataCapacity_}, _fileNamePattern {std::move(fileNamePattern_)},
        _isCollecting {}, _capacityBreached {}, _flushInterval{5} {
          this->periodicFlush(_flushInterval);
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

    void periodicFlush(unsigned int flushInterval_);

    private:

    void persistSamples(int fd_, const probes::Sample* begin_, const probes::Sample* end_);
    std::tuple<int, int, int> collectSamples(SamplesBuffer* buffer_);
    std::tuple<int, int> flush(SamplesBuffer* buffer_);

    StorageMgr _storageMgr;
    std::string _fileNamePattern;
    bool _isCollecting;
    bool _capacityBreached;
    unsigned int _flushInterval;
  };

}}
