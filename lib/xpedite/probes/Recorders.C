////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite Probes - Probes with near zero overhead, that can be activated at runtime
//
// This file contains the declarations for diferent recorders.
//
// ExpandAndrecord - expand the samples buffer and record tsc
// recordAndLog    - record tsc and log probe details
// record          - record tsc
// recordPmc       - record tsc, fixed and general performance counters
// recordPerfEvents  - record tsc, pmu events using linux perf events api
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/probes/ProbeList.H>
#include <xpedite/probes/Recorders.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/log/Log.H>

extern "C" {

  void XPEDITE_CALLBACK xpediteExpandAndRecord(const void* returnSite_, uint64_t tsc_) {
    using namespace xpedite::probes;
    if(XPEDITE_UNLIKELY(samplesBufferPtr >= samplesBufferEnd)) {
      xpedite::framework::SamplesBuffer::expand();
    }
    if(XPEDITE_LIKELY(samplesBufferPtr < samplesBufferEnd)) {
      new (samplesBufferPtr) Sample {returnSite_, tsc_};
      samplesBufferPtr = samplesBufferPtr->next();
    }
  }

  void XPEDITE_CALLBACK xpediteExpandAndRecordWithData(const void* returnSite_, uint64_t tsc_, __uint128_t data_) {
    using namespace xpedite::probes;
    if(XPEDITE_UNLIKELY(samplesBufferPtr >= samplesBufferEnd)) {
      xpedite::framework::SamplesBuffer::expand();
    }
    if(XPEDITE_LIKELY(samplesBufferPtr < samplesBufferEnd)) {
      new (samplesBufferPtr) Sample {returnSite_, tsc_, data_};
      samplesBufferPtr = samplesBufferPtr->next();
    }
  }

  void XPEDITE_CALLBACK xpediteRecordAndLog(const void* returnSite_, uint64_t tsc_) {
    // Not for use in crit path, for troubleshooting only
    using namespace xpedite::probes;
    xpediteExpandAndRecord(returnSite_, tsc_);
    if(auto probe = probeList().findByReturnSite(getcallSite(returnSite_))) {
      XpediteLogInfo << "Recording " << probe->toString() << " | timestamp - " << tsc_<< XpediteLogEnd;
    }
    else {
      XpediteLogInfo << "Recording from call site " << std::hex << getcallSite(returnSite_) << std::dec 
        << " | timestamp - " << tsc_<< XpediteLogEnd;
    }
  }

  void XPEDITE_CALLBACK xpediteRecordWithDataAndLog(const void* returnSite_, uint64_t tsc_, __uint128_t data_) {
    // Not for use in crit path, for troubleshooting only
    using namespace xpedite::probes;
    xpediteExpandAndRecordWithData(returnSite_, data_, tsc_);
    if(auto probe = probeList().findByReturnSite(getcallSite(returnSite_))) {
      XpediteLogInfo << "Recording (with data*) " << probe->toString() << " | timestamp - " << tsc_<< XpediteLogEnd;
    }
    else {
      XpediteLogInfo << "Recording (with data*) from call site " << std::hex << getcallSite(returnSite_) << std::dec 
        << " | timestamp - " << tsc_<< XpediteLogEnd;
    }
  }

  void XPEDITE_CALLBACK xpediteRecord(const void* returnSite_, uint64_t tsc_) {
    using namespace xpedite::probes;
    if(XPEDITE_LIKELY(samplesBufferPtr < samplesBufferEnd)) {
      new (samplesBufferPtr) Sample {returnSite_, tsc_};
      samplesBufferPtr = samplesBufferPtr->next();
    }
  }

  void XPEDITE_CALLBACK xpediteRecordWithData(const void* returnSite_, uint64_t tsc_, __uint128_t data_) {
    using namespace xpedite::probes;
    if(XPEDITE_LIKELY(samplesBufferPtr < samplesBufferEnd)) {
      new (samplesBufferPtr) Sample {returnSite_, tsc_, data_};
      samplesBufferPtr = samplesBufferPtr->next();
    }
  }

  void XPEDITE_CALLBACK xpediteRecordPmc(const void* returnSite_, uint64_t tsc_) {
    using namespace xpedite::probes;
    if(XPEDITE_UNLIKELY(samplesBufferPtr >= samplesBufferEnd)) {
      xpedite::framework::SamplesBuffer::expand();
    }
    if(XPEDITE_LIKELY(samplesBufferPtr < samplesBufferEnd)) {
      new (samplesBufferPtr) Sample {returnSite_, tsc_, true};
      samplesBufferPtr = samplesBufferPtr->next();
    }
  }

  void XPEDITE_CALLBACK xpediteRecordPmcWithData(const void* returnSite_, uint64_t tsc_, __uint128_t data_) {
    using namespace xpedite::probes;
    if(XPEDITE_UNLIKELY(samplesBufferPtr >= samplesBufferEnd)) {
      xpedite::framework::SamplesBuffer::expand();
    }
    if(XPEDITE_LIKELY(samplesBufferPtr < samplesBufferEnd)) {
      new (samplesBufferPtr) Sample {returnSite_, tsc_, data_, true};
      samplesBufferPtr = samplesBufferPtr->next();
    }
  }

  void XPEDITE_CALLBACK xpediteRecordPerfEvents(const void* returnSite_, uint64_t tsc_) {
    using namespace xpedite::probes;
    using namespace xpedite::pmu;
    using namespace xpedite::framework;
    if(XPEDITE_UNLIKELY(samplesBufferPtr >= samplesBufferEnd)) {
      xpedite::framework::SamplesBuffer::expand();
    }
    if(XPEDITE_LIKELY(samplesBufferPtr < samplesBufferEnd)) {
      new (samplesBufferPtr) Sample {returnSite_, tsc_, SamplesBuffer::samplesBuffer()->perfEvents()};
      samplesBufferPtr = samplesBufferPtr->next();
    }
  }

  void XPEDITE_CALLBACK xpediteRecordPerfEventsWithData(const void* returnSite_, uint64_t tsc_, __uint128_t data_) {
    using namespace xpedite::probes;
    using namespace xpedite::pmu;
    using namespace xpedite::framework;
    if(XPEDITE_UNLIKELY(samplesBufferPtr >= samplesBufferEnd)) {
      xpedite::framework::SamplesBuffer::expand();
    }
    if(XPEDITE_LIKELY(samplesBufferPtr < samplesBufferEnd)) {
      new (samplesBufferPtr) Sample {returnSite_, tsc_, data_, SamplesBuffer::samplesBuffer()->perfEvents()};
      samplesBufferPtr = samplesBufferPtr->next();
    }
  }
}
