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
#include <xpedite/persistence/Persister.H>
#include <xpedite/persistence/SegmentLoader.H>
#include <xpedite/persistence/ProfileInfo.H>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <ostream>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace xpedite::persistence {

  class SamplesLoader
  {
    int _fd;
    const FileHeader* _fileHeader;
    const SamplesSegment* _samplesSegment;
    unsigned _size;

    const void* samplesEnd() const noexcept {
      return reinterpret_cast<const char*>(_fileHeader) + _size;
    }

    SamplesLoader(const SamplesLoader&)            = delete;
    SamplesLoader& operator=(const SamplesLoader&) = delete;

    public:

    SamplesLoader(SamplesLoader&& other_)
      : _fd {other_._fd}, _fileHeader {other_._fileHeader }, _samplesSegment {other_._samplesSegment },
        _size {other_._size } {
      other_._fd = {};
      other_._fileHeader = {};
      other_._samplesSegment = {};
      other_._size = {};
    }

    SamplesLoader& operator=(SamplesLoader&& other_) {
      std::swap(_fd, other_._fd);
      std::swap(_fileHeader, other_._fileHeader);
      std::swap(_samplesSegment, other_._samplesSegment);
      std::swap(_size, other_._size);
      return *this;
    }

    class Iterator : public std::iterator<std::input_iterator_tag, const probes::Sample>
    {
      const probes::Sample* _samples;
      const void* _end;
      unsigned _size;

      public:

      explicit Iterator(const SamplesSegment* samplesSegment_, const void* end_)
        : _samples {reinterpret_cast<const probes::Sample*>(end_)}, _end {end_}, _size {} {
        if(samplesSegment_ < _end) {
          std::tie(_samples, _size) = samplesSegment_->data();
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
            auto samplesHeader = reinterpret_cast<const SamplesSegment*>(_samples);
            std::tie(_samples, _size) = samplesHeader->data();
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
      : _fd {}, _fileHeader {}, _samplesSegment {}, _size {} {
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
      _samplesSegment = _fileHeader->samples();
    }

    ProfileInfo loadProfileInfo(std::vector<ux::UxProbe> uxProbes_ = {}) noexcept {
      SegmentLoader segmentLoader {};
      _fileHeader->forEachMetaSegment(segmentLoader);
      auto [cpuInfo, probeInfoMap, events, topdownNodes, metrics] = segmentLoader.data();
      auto probeHandles {std::make_shared<ProbeHandles>(probeInfoMap, std::move(uxProbes_))};
      return ProfileInfo {
        cpuInfo, std::move(probeHandles), std::move(events), std::move(topdownNodes), std::move(metrics)
      };
    }

    uint32_t pmcCount() const noexcept { return _fileHeader->pmcCount(); }
    Iterator begin() const { return Iterator {_samplesSegment, samplesEnd()}; }
    Iterator end() const   { return Iterator {samplesEnd(), samplesEnd()};   }

    uint64_t tscHz() const noexcept {
      if(_fileHeader) {
        return _fileHeader->tscHz();
      }
      return {};
    }

    static int saveAsCsv(const char* samplesPath_, const char* dest_);
    static int streamAsCsv(const char* samplesPath_, std::ostream& destStream_);
  };

}
