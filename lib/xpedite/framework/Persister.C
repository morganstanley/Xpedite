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
#include <memory>

namespace xpedite { namespace framework {

  static unsigned batchCount;

  std::vector<CallSiteInfo> buildCallSiteList() {
    std::vector<CallSiteInfo> callSites;
    for(auto& probe : probes::probeList()) {
      callSites.emplace_back(probe.rawRecorderCallSite(), probe.attr(), probe.id());
    }
    return callSites;
  }

  void persistHeader(int fd_) {
    static auto tscHz = util::estimateTscHz();
    auto callSites = buildCallSiteList();
    timeval  time;
    gettimeofday(&time, nullptr);
    auto capacity = FileHeader::capacity(callSites.size());
    std::unique_ptr<char []> buffer {new char[capacity]};
    new (buffer.get()) FileHeader {callSites, time, tscHz, pmu::pmuCtl().pmcCount()};
    write(fd_, buffer.get(), capacity);
    XpediteLogInfo << "persisted file header with " << callSites.size() << " call sites  | capacity "
      << sizeof(FileHeader) << " + " << FileHeader::callSiteSize(callSites.size()) << " = "
      << capacity << " bytes" << XpediteLogEnd;
  }

  void persistData(int fd_, const probes::Sample* begin_, const probes::Sample* end_) {

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
