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
#include <string>

namespace xpedite { namespace framework {

  class Collector
  {
    public:

    Collector(std::string fileNamePattern_)
      : _fileNamePattern {std::move(fileNamePattern_)}, _isCollecting {} {
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

    std::string _fileNamePattern;
    bool _isCollecting;
  };

}}
