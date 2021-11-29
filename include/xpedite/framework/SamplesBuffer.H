////////////////////////////////////////////////////////////////////////////////////////////
//
// SamplesBuffer - Finite size wait free buffer for sample collection
//
// Each thread during initialization, creates an instance of SamplesBuffer.
// The buffer instances are chained with one another, to form a linked list.
//
// The class also supports ability to attach framework thread to a sample buffer chain.
// The framework thread, periodically polls buffers for new sample data.
// Intact sample objects are copied to release space in the samples buffer.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <xpedite/util/Util.H>
#include <xpedite/common/WaitFreeBufferPool.H>
#include <xpedite/probes/Config.H>
#include <xpedite/probes/Sample.H>
#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/framework/Persister.H>
#include <xpedite/log/Log.H>
#include <atomic>
#include <stdlib.h>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include <tuple>
#include <atomic>
#include <string>
#include <sstream>
#include <iomanip>

extern __thread xpedite::probes::Sample* samplesBufferPtr;
extern __thread xpedite::probes::Sample* samplesBufferEnd;

namespace xpedite { namespace perf { namespace test {
  struct Override;
}}}

namespace xpedite { namespace framework {

  class SamplesBuffer : public util::AlignedObject<common::ALIGNMENT>
  {
    public:

    static SamplesBuffer* allocate() {
      return new SamplesBuffer {};
    }

    static void deallocate(SamplesBuffer* buffer_) {
      delete buffer_;
    }

    static SamplesBuffer* head() noexcept {
      return _head.load(std::memory_order_relaxed);
    }

    static bool attachAll(Persister& persister_, const std::string& fileNamePattern_) noexcept {
      auto begin = SamplesBuffer::head();
      auto buffer = begin;
      while(buffer) {
        if(!buffer->attachReader(persister_, fileNamePattern_)) {
          break;
        }
        buffer = buffer->next();
      }

      if(buffer) {
        auto cursor = begin;
        while(cursor != buffer) {
          cursor->detachReader();
          cursor = cursor->next();
        }
        return false;
      }
      return true;
    }

    static bool detachAll() noexcept {
      bool status {true};
      auto buffer = SamplesBuffer::head();
      while(buffer) {
        status &= buffer->detachReader();
        buffer = buffer->next();
      }
      return status;
    }

    static bool isInitialized();
    static SamplesBuffer* samplesBuffer();
    static void expand();
    
    bool isReaderAttached() const noexcept {
      return _fd >= 0;
    }

    bool attachReader(Persister& persister_, const std::string& fileNamePattern_) noexcept {
      if(isReaderAttached()) {
        XpediteLogError << "xpedite - failed to attach reader to thread " << tid() 
          << " - reader already attached. attaching multiple readers not permitted" << XpediteLogEnd;
        return false;
      }

      std::string filePath = buildSampledFilePath(fileNamePattern_);
      _fd = util::openSamplesFile(filePath);
      if(_fd < 0) {
        XpediteLogError << "xpedite - failed to attach reader to thread " << tid() << " - cannot open file - \"" 
          << filePath << "\"" << XpediteLogEnd;
        return false;
      }

      persister_.persistHeader(_fd);
      uint64_t rindex, windex;
      std::tie(rindex, windex) = _bufferPool.attachReader();
      XpediteLogInfo << "xpedite - attached reader to thread - " << tid() << " | buffer index state - [readIndex - "
        << rindex << " / write index - " << windex <<  "] | sample file " << filePath << " | fd - " << _fd << XpediteLogEnd;
      return true;
    }

    bool detachReader() noexcept {
      if(!isReaderAttached()) {
        XpediteLogError << "xpedite - failed to detach reader to thread " << tid() 
          << " - thread is not attached to any active readers" << XpediteLogEnd;
        return false;
      }

      close(_fd);
      uint64_t rindex, windex;
      std::tie(rindex, windex) = _bufferPool.detachReader();
      XpediteLogInfo << "xpedite - detached reader from thread - " << tid() << " | buffer index state - [readIndex - "
        << rindex << " / write index - " << windex <<  "] | fd - " << _fd << XpediteLogEnd;
      _fd = -1;
      return true;
    }

    SamplesBuffer* next() noexcept {
      return _next;
    }

    const SamplesBuffer* next() const noexcept {
      return _next;
    }

    std::tuple<probes::Sample*, probes::Sample*> nextWritableRange() noexcept {
      auto begin = _bufferPool.nextWritableBuffer();
      auto end = begin  + bufferGuardOffset;
      return std::make_tuple(begin, end);
    }

    std::tuple<const probes::Sample*, const probes::Sample*> nextReadableRange() noexcept {
      _curReadBuf = _bufferPool.nextReadableBuffer(_curReadBuf);
      const probes::Sample* end {_curReadBuf ? _curReadBuf + bufferGuardOffset : nullptr};
      return std::make_tuple(_curReadBuf, end);
    }

    std::tuple<const probes::Sample*, const probes::Sample*> peekWithDataRace() const noexcept {
      auto begin =  _bufferPool.peekWithDataRace();
      auto end = begin  + bufferGuardOffset;
      return std::make_tuple(begin, end);
    }

    uint64_t overflowCount() noexcept {
      auto ofCount = _bufferPool.overflowCount();
      auto c = ofCount - _lastOverflowCount;
      _lastOverflowCount = ofCount;
      return c;
    }

    pid_t tid()               const noexcept { return _tid;            }
    uint64_t lastSampledTsc() const noexcept { return _lastSampledTsc; }
    int fd()                  const noexcept { return _fd;             }

    void setLastSampledTsc(uint64_t lastSampledTsc_) noexcept {
      _lastSampledTsc = lastSampledTsc_;
    }

    const perf::PerfEventSet* perfEvents() const noexcept {
      return _perfEventSet.load(std::memory_order_acquire);
    }

    void updatePerfEvents(perf::PerfEventSet* perfEventSet_) noexcept {
      return _perfEventSet.store(perfEventSet_, std::memory_order_release);
    }

    private:

    static  uint64_t tlsAddr() noexcept {
      uint64_t addr;
      asm("movq %%fs:0, %0" : "=r"(addr));
      return addr;
    }

    std::string buildTidStr() noexcept {
      std::ostringstream stream;
      stream << _tid << "-" << std::setw(16) << std::setfill('0') << std::hex << _tlsAddr << std::dec;
      return stream.str();
    }

    SamplesBuffer() noexcept
      : _bufferPool {}, _fd {-1}, _tid {util::gettid()}, _tlsAddr {tlsAddr()}, _tidStr {buildTidStr()}, _curReadBuf {}
      , _lastSampledTsc {} , _lastOverflowCount {}, _perfEventSet {} {
      SamplesBuffer* next = _head.load(std::memory_order_relaxed);
      do {
        _next = next;
      } while(!_head.compare_exchange_weak(next, this, std::memory_order_release, std::memory_order_relaxed));
      pmu::pmuCtl().attachPerfEvents(this);
    }

    std::string buildSampledFilePath(const std::string& fileNamePattern_) const {
      std::string fileName = fileNamePattern_;
      auto index = fileName.find("*");
      if(index != std::string::npos) {
        fileName.replace(index, 1, _tidStr);
      }
      return fileName;
    }

    friend class pmu::PmuCtl;
    friend struct perf::test::Override;

    static std::atomic<SamplesBuffer*> _head;
    static constexpr size_t bufferSize = 4 * 1024;
    static constexpr size_t poolSize   = 16;
    static constexpr size_t bufferGuardSize = (probes::Sample::maxSize() * 4) / sizeof(probes::Sample);
    using BufferPool = common::WaitFreeBufferPool<probes::Sample, bufferSize, poolSize>;
    static constexpr size_t bufferGuardOffset = BufferPool::getBufferSize() - bufferGuardSize;

    BufferPool _bufferPool;
    SamplesBuffer* _next;
    int _fd;
    const pid_t _tid;
    const uint64_t _tlsAddr;
    const std::string _tidStr;
    const probes::Sample* _curReadBuf;
    uint64_t _lastSampledTsc;
    uint64_t _lastOverflowCount;

    alignas(common::ALIGNMENT) std::atomic<perf::PerfEventSet*> _perfEventSet;

  };

}}
