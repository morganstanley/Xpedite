///////////////////////////////////////////////////////////////////////////////
//
// Methods to persist timing and pmc data to filesystem
// The records can be encoded in one these three formats (text, csv, binary) 
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Persister.H>
#include <xpedite/probes/Config.H>
#include <xpedite/probes/ProbeList.H>
#include <xpedite/probes/Sample.H>
#include <xpedite/util/Util.H>
#include <xpedite/util/Tsc.H>
#include <xpedite/pmu/PMUCtl.H>
#include <sys/time.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

namespace xpedite { namespace framework {

  static unsigned batchCount;

  void Persister::resizeBuffer(size_t objSize_) {
    auto size = _buffer.size();
    while(size < _hdr->size() + objSize_) {
      size *= 2;
    }
    _buffer.resize(size);
    _hdr = reinterpret_cast<FileHeader*>(_buffer.data());
  }

  Persister::Persister()
    : _hdr {}, _buffer {} {
    auto tscHz = util::estimateTscHz();
    timeval  time;
    gettimeofday(&time, nullptr);
    _buffer.resize(2 * 1024 * 1024);
    _hdr = new (_buffer.data()) FileHeader {time, tscHz, pmu::pmuCtl().pmcCount()};
    for(auto& probe : probes::probeList()) {
      Name probeName    {probe.name(), static_cast<uint32_t>(strlen(probe.name()))+1};
      Name fileName     {probe.file(), static_cast<uint32_t>(strlen(probe.file()))+1};
      Name functionName {probe.func(), static_cast<uint32_t>(strlen(probe.func()))+1};
      auto objSize =  sizeof(ProbeInfo) + probeName._size + fileName._size + functionName._size;
      if(freeSize() < objSize) {
        resizeBuffer(objSize);
      }
      _hdr->add(
        probe.rawRecorderCallSite(), probe.attr(), probe.id(), probeName, fileName, functionName, probe.line()
      );
    }
  }

  void Persister::persistHeader(int fd_) {
    write(fd_, _buffer.data(), _hdr->size());
    XpediteLogInfo << "persisted file header with " << _hdr->probeCount() << " call sites  | capacity "
      << _hdr->size() << " bytes" << XpediteLogEnd;
  }

  void Persister::persistData(int fd_, const probes::Sample* begin_, const probes::Sample* end_) {

    if(!begin_ || begin_ == end_) {
      return;
    }
    uint64_t ccstart {RDTSC()};
    timeval  time;
    gettimeofday(&time, nullptr);
    unsigned size = reinterpret_cast<const char*>(end_) - reinterpret_cast<const char*>(begin_);

    SegmentHeader segmentHeader{time, size, ++batchCount}; 
    write(fd_, &segmentHeader, sizeof(segmentHeader));
    write(fd_, begin_, size);
    if(probes::config().verbose()) {
      XpediteLogInfo << "persisted segment " << size << " bytes in " << RDTSC() - ccstart << " cycles" << XpediteLogEnd;
    }
  }

}}
