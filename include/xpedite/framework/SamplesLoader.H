////////////////////////////////////////////////////////////////////////////////////
//
// SamplesLoader loads probe sample data from binary files
//
// Xpedite probes store timing and performance counter data using variable 
// length POD objects. A collection of sample objects is grouped and written
// as a batch. 
//
// The loader iterates through the POD collection,  to extract 
// records in string format for consumption by the profiler
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <xpedite/util/Errno.H>
#include <xpedite/framework/Persister.H>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace xpedite { namespace framework {

  class SamplesLoader
  {
    int _fd;
    const FileHeader* _fileHeader;
    CallSiteMap _callSiteMap;
    const SegmentHeader* _segmentHeader;
    unsigned _size;

    const void* samplesEnd() const noexcept {
      return reinterpret_cast<const char*>(_fileHeader) + _size;
    }

    SamplesLoader(const SamplesLoader&)            = delete;
    SamplesLoader& operator=(const SamplesLoader&) = delete;
    SamplesLoader(SamplesLoader&&)                 = delete;
    SamplesLoader& operator=(SamplesLoader&&)      = delete;

    public:

    class Iterator : public std::iterator<std::input_iterator_tag, const probes::Sample>
    {
      const probes::Sample* _samples;
      const void* _end;
      unsigned _size;

      public:

      explicit Iterator(const SegmentHeader* samplesHeader_, const void* end_)
        : _samples {reinterpret_cast<const probes::Sample*>(end_)}, _end {end_}, _size {} {
        if(samplesHeader_ < _end) {
          std::tie(_samples, _size) = samplesHeader_->samples();
        }
      }

      explicit Iterator(const void* begin_, const void* end_)
        : _samples {reinterpret_cast<const probes::Sample*>(begin_)}, _end {end_}, _size {} {
      }

      Iterator& operator++() {
        if(_samples < _end) {
          _size -= _samples->size();
          _samples = _samples->next();
          if(!_size && _samples < _end) {
            auto samplesHeader = reinterpret_cast<const SegmentHeader*>(_samples);
            std::tie(_samples, _size) = samplesHeader->samples();
          }
        }
        return *this;
      }

      Iterator operator++(int) {
        Iterator i {*this};
        operator++();
        return i;
      }

      bool operator==(Iterator other_) const {
        return _samples == other_._samples && _end == other_._end;
      }

      bool operator!=(Iterator other_) const {
        return !(*this == other_);
      }

      reference operator*() const {
        return *_samples;
      }
    };

    explicit SamplesLoader(const char* path_)
      : _fd {}, _fileHeader {}, _callSiteMap {}, _segmentHeader {}, _size {} {
      load(path_);
    }

    ~SamplesLoader() {
      if(_fileHeader) {
        munmap(const_cast<FileHeader*>(_fileHeader), _size);
      }
      if(_fd) {
        close(_fd);
      }
    }

    std::string errorMsg(const char* msg_) {
      util::Errno e;
      std::ostringstream os;
      os << msg_ << " - " << e.asString();
      return os.str();
    }

    void load(const char* path_) {
      int _fd = open(path_, O_RDONLY);
      if (_fd < 0) {
        throw std::runtime_error {errorMsg("failed to open samples file")};
      }

      struct stat buf;
      if(fstat(_fd, &buf)) {
        throw std::runtime_error {errorMsg("failed to stat samples file")};
      }
      _size = buf.st_size;

      char* ptr {};
      if((ptr = static_cast<char*>(mmap(nullptr, _size, PROT_READ, MAP_SHARED, _fd, 0))) == MAP_FAILED) {
        throw std::runtime_error {errorMsg("failed to mmap file")};
      }

      _fileHeader = reinterpret_cast<const FileHeader*>(ptr);
      if(!_fileHeader->isValid()) {
        throw std::runtime_error {errorMsg("detected data corruption - mismatch in header signature")};
      }

      const CallSiteInfo* callSites;
      uint64_t callSiteCount;
      std::tie(callSites, callSiteCount) = _fileHeader->callSites();
      for(unsigned i=0; i<callSiteCount; ++i) {
        _callSiteMap.add(callSites[i]);
      }
      _segmentHeader = _fileHeader->segmentHeader();
    }

    const CallSiteInfo* locateCallSite(const void* callSite_) const noexcept {
      return _callSiteMap.locateInfo(callSite_);
    }

    uint32_t pmcCount()             const noexcept { return _fileHeader->pmcCount(); }
    const CallSiteMap callSiteMap() const noexcept { return _callSiteMap;            }

    Iterator begin() const { return Iterator {_segmentHeader, samplesEnd()}; }
    Iterator end() const   { return Iterator {samplesEnd(), samplesEnd()};   }

    uint64_t tscHz() const noexcept {
      if(_fileHeader) {
        return _fileHeader->tscHz();
      }
      return {};
    }
  };

}}
