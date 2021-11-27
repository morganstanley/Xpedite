//////////////////////////////////////////////////////////////////////////////////////////
//
// Collector - collects probe data captured by application threads
//
// The collector provides logic to poll and collect probe data from sample buffers 
// The samples are written to a per thread wait free circular buffer.
// Collector functions as a cosumer and copies sample data, to make reoom for new ones.
// The copied data is persisted for use by the profiler.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
//////////////////////////////////////////////////////////////////////////////////////////

#include "Collector.H"
#include <xpedite/util/Util.H>
#include <xpedite/framework/Persister.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <xpedite/log/Log.H>
#include <tuple>

namespace xpedite { namespace framework {

  bool Collector::beginSamplesCollection() {
    XpediteLogInfo << "xpedite - begin out of band samples collection" << XpediteLogEnd;
    _isCollecting = SamplesBuffer::attachAll(_persister, _fileNamePattern);
    return _isCollecting;
  }

  bool Collector::endSamplesCollection() {
    XpediteLogInfo << "xpedite - end out of band samples collection" << XpediteLogEnd;
    if(isCollecting()) {
      poll(true);
      _isCollecting = false;
      return SamplesBuffer::detachAll();
    }
    return false;
  }

  void Collector::persistSamples(int fd_, const probes::Sample* begin_, const probes::Sample* end_) {
    auto size = reinterpret_cast<const char*>(end_) - reinterpret_cast<const char*>(begin_);
    if(_storageMgr.consume(size)) {
      _persister.persistData(fd_, begin_, end_);
    } else if(!_capacityBreached) {
      // capacity breached - dropping all samples from now on
      _capacityBreached = true;
      XpediteLogInfo << "Dropping this and future samples - max samples data capacity (" << _storageMgr.consumption() << " out of "
        << _storageMgr.capacity() << ") consumed." << XpediteLogEnd;
    }
  }

  void checkOverflow(pid_t tid_, const probes::Sample* cursor_, const probes::Sample* end_) {
    auto overflow = reinterpret_cast<const char*>(cursor_) - reinterpret_cast<const char*>(end_);
    if(overflow >= probes::Sample::maxSize()) {
      std::ostringstream stream;
      stream << "xpedite - detected buffer overflow (" << overflow << " bytes), while collecting samples from "
        << "thread " << tid_ << ". max threshold " << probes::Sample::maxSize() << " bytes.";
      auto errMsg  = stream.str();
      XpediteLogCritical << errMsg << XpediteLogEnd;
      throw std::runtime_error {errMsg};
    }
  }

  std::tuple<int, int, int> Collector::collectSamples(SamplesBuffer* buffer_) {
    int bufferCount {}, sampleCount {}, staleSampleCount {};

    while(true) {
      const probes::Sample *begin, *end;
      std::tie(begin, end) = buffer_->nextReadableRange();
      if(!begin)
        break;

      int perBufferSampleCount {};
      auto cursor = begin;
      while(cursor < end) {
        if(cursor->tsc() <= buffer_->lastSampledTsc()) {
          cursor = cursor->next();
          begin = cursor;
          staleSampleCount += perBufferSampleCount + 1;
          perBufferSampleCount = 0;
        }
        else {
          ++perBufferSampleCount;
          buffer_->setLastSampledTsc(cursor->tsc());
          cursor = cursor->next();
        }
      }

      if(begin < cursor) {
        checkOverflow(buffer_->tid(), cursor, end);
        persistSamples(buffer_->fd(), begin, cursor);
        sampleCount += perBufferSampleCount;
        ++bufferCount;
      }
    }
    return std::make_tuple(bufferCount, sampleCount, staleSampleCount);
  }

  std::tuple<int, int> Collector::flush(SamplesBuffer* buffer_) {
    uint64_t minTsc {}, maxTsc = RDTSC();
    const probes::Sample *begin, *end;
    std::tie(begin, end) = buffer_->peekWithDataRace();

    // The buffer has a race with the writer thread.
    // Need to validate each sample for consistency before persistance
    
    int sampleCount {}, staleSampleCount {};
    auto cursor = begin;
    while(cursor < end) {
      auto tsc = cursor->tsc();
      if(tsc <= minTsc || tsc >= maxTsc) {
        break;
      }

      if(tsc <= buffer_->lastSampledTsc()) {
        cursor = cursor->next();
        begin = cursor;
        staleSampleCount += sampleCount + 1;
        sampleCount = 0;
      }
      else {
        ++sampleCount;
        buffer_->setLastSampledTsc(tsc);
        cursor = cursor->next();
      }
      minTsc = tsc;
    }

    if(begin < cursor) {
      checkOverflow(buffer_->tid(), cursor, end);
      XpediteLogInfo << "xpedite - collector flushed samples - [valid - " << sampleCount << ", stale - " << staleSampleCount << "]" << XpediteLogEnd;
      persistSamples(buffer_->fd(), begin, cursor);
    }
    return std::make_tuple(sampleCount, staleSampleCount);
  }

  void Collector::poll(bool flush_) {
    if(isCollecting()) {
      //thread_local int pollCount;
      auto buffer = SamplesBuffer::head();
      int threadCount {}, bufferCount {}, sampleCount {}, staleSampleCount {}, overflowCount {};
      while(buffer) {
        if(!buffer->isReaderAttached()) {
          //TODO, have to limit the number of attach operations attempted
          buffer->attachReader(_persister, _fileNamePattern);
        }

        if(buffer->isReaderAttached()) {
          int curBufferCount {}, curSampleCount {}, curStaleSampleCount {};
          std::tie(curBufferCount, curSampleCount, curStaleSampleCount) = collectSamples(buffer);
          bufferCount += curBufferCount;
          sampleCount += curSampleCount;
          staleSampleCount += curStaleSampleCount;

          if(flush_) {
            std::tie(curSampleCount, curStaleSampleCount) = flush(buffer);
            if(curSampleCount) {
              sampleCount += curSampleCount;
              staleSampleCount += curStaleSampleCount;
              ++bufferCount;
            }
          }
          if(curBufferCount || curSampleCount) ++threadCount; 
          overflowCount += buffer->overflowCount();
        }
        buffer = buffer->next();
      }

      if(overflowCount) {
        XpediteLogWarning << "xpedite - detected loss of samples from " << overflowCount << " buffer(s)" << XpediteLogEnd;
      }

      if(sampleCount) {
        XpediteLogInfo << "xpedite - collector polled samples - [valid - " << sampleCount << ", stale - " << staleSampleCount
          << "] | buffers - " << bufferCount  << " | " << "threads - " << threadCount << XpediteLogEnd;
      }
    }
  }

}}
