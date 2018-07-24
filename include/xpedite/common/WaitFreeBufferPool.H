//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WaitFreeBufferPool - a pool of buffers, that is thread safe to be used between two threads 
// (one writer and one reader). The pool is designed to never stop writer thread from making progress
// The writer borrows a free buffer for storing data, which could be potentially consumed by reader, 
// at a later time.
// 
// The writer on consuming the free buffer, will query the pool for next writable buffer.
// 
// If the reader keeps up with the pace of the writer, the pool will be able to get other free buffers
// for the writer to write next.
// 
// However, if the reader is lagging behind, all the free buffers gets filled up eventually and the 
// pool will fulfill requests from writer thread with the buffer that was recently written by the 
// writer. Thus, a slow reader can cause loss of data.
// 
// The reader thread, can query the pool for next readable buffer. If there is data to consume, 
// the pool will return a pointer to buffer with data, else a nullptr is returned.
// 
// Threadsafety and memory visibity is guranteed for writer and read to write and read data 
// respectively.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/util/Allocator.H>
#include <xpedite/platform/Builtins.H>
#include <array>
#include <atomic>
#include <memory>
#include <cassert>
#include <type_traits>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <tuple>

namespace xpedite { namespace common {

  template<typename T, size_t Size>
  struct Buffer
  {
    Buffer()
      : _data {} { // prefault the buffer, after allocation
    }

    static void* operator new(size_t size_) {
      void *ptr = util::xpediteMalloc(size_);
      if(!ptr) {
        throw std::bad_alloc {};
      }
      return ptr;
    }

    static void* operator new[](size_t size_) {
      void *ptr = util::xpediteMalloc(size_);
      if(!ptr) {
        throw std::bad_alloc {};
      }
      return ptr;
    }

    static void operator delete(void* ptr_, size_t size_) {
      util::xpediteFree(ptr_, size_);
    }

    static void operator delete[](void* ptr_, size_t size_) {
      util::xpediteFree(ptr_, size_);
    }

    std::array<T, Size>& data() noexcept {
      return _data;
    }

    private:
    std::array<T, Size> _data;
  };

  inline constexpr bool isPoolSizeValid(unsigned poolSize_) {
    return poolSize_ > 1 && (poolSize_ & (poolSize_ -1)) == 0;
  }

  constexpr int ALIGNMENT {XPEDITE_CACHELINE_SIZE}; // align to cache line

  template <typename T, unsigned bufferSize, unsigned poolSize, typename = void>
  class WaitFreeBufferPool;
  
  template <typename T, unsigned bufferSize, unsigned poolSize>
  class WaitFreeBufferPool<T, bufferSize, poolSize, typename std::enable_if<isPoolSizeValid(poolSize)>::type>
    : public util::AlignedObject<ALIGNMENT>
  {
    using Pool = Buffer<T, bufferSize * poolSize>;
    public:

      static constexpr unsigned getBufferSize() noexcept {
        return bufferSize;
      }

      WaitFreeBufferPool() 
        // The base class check for alignment and can throw, runtime exception
        : _writeIndex {}, _readIndex {readIndexMax}, _pool {new Pool {}}, _overflowCount {}, _{} {
      }

      std::tuple<uint64_t, uint64_t> attachReader() noexcept {
        auto windex = _writeIndex.load(std::memory_order_relaxed);
        uint64_t rindex {};
        do {
          rindex = windex ? windex -1 : 0;
          _readIndex.store(rindex, std::memory_order_seq_cst);
          windex = _writeIndex.load(std::memory_order_relaxed);
        } while(XPEDITE_UNLIKELY(windex > rindex + poolSize));
        return std::make_tuple(rindex, windex);
      }

      std::tuple<uint64_t, uint64_t> detachReader() noexcept {
        compilerBarrier();
        auto rindex = _readIndex.load(std::memory_order_relaxed);
        auto windex = _writeIndex.load(std::memory_order_relaxed);
        _readIndex.store(readIndexMax, std::memory_order_relaxed);
        return std::make_tuple(rindex, windex);
      }

      // will always return a buffer for writer to write to
      T* nextWritableBuffer() noexcept {
        auto windex = _writeIndex.load(std::memory_order_relaxed);
        auto rindex = _readIndex.load(std::memory_order_relaxed);

        /********************************************************************
        ** what happens, when rindex + poolSize overflows ?
        ** The inequality "windex < rindex + poolSize" is always false.
        ** The branch is always taken, preventing windex from getting
        ** incrmented. 
        ** Thus, an rindex, close to overflow, stops pooling of buffers.
        ** It also prevents windex from ever overflowing.
        **
        ** Is this desirable ?
        ** Of course not. However, it takes quite a while for a 64 bit counter 
        ** to overflow. This implementation assumes, overflow is not a real
        ** use case scenario for productions applications that will use this
        ** pool for probe sample collection.
        ** If this ever gets repurposed for someother use, this assumption
        ** might have to be revisited again.
        ********************************************************************/
        if(XPEDITE_LIKELY(windex < rindex + poolSize)) {
          ++windex;

          /******************************************************************
          ** prevent previous stores from getting re-ordered beyond this point.
          ** stores won't get re-ordered with respect to other stores in IA32e. 
          ** however the compiler is free re-order them, provided there are no
          ** architectural or program order data dependencies. 
          ** Need release barrier to prevent compiler, from reorderening stores 
          ** that lack architectural dependencies.
          *******************************************************************/
          _writeIndex.store(windex, std::memory_order_release);
        }
        else {
          ++_overflowCount;
        }
        return bufferAt(windex);
      }

      // will return a buffer if and only if data is available for reading
      const T* nextReadableBuffer(const T* curReadBuf_) noexcept {
        auto rindex = _readIndex.load(std::memory_order_relaxed);
        if(XPEDITE_LIKELY(curReadBuf_ != nullptr)) {
          ++rindex;
          assert(curReadBuf_ == bufferAt(rindex));

          /******************************************************************
          ** prevent previous loads from getting re-ordered beyond this point.
          ** In IA32e, load can only be re-ordered with previous stores.
          ** however the compiler is free re-order instructions, provided there 
          ** are no architectural or program order data dependencies. 
          ** Need either compiler barrier or sequential consistency semantics
          ** to prevent previous loads, that lack architectural dependencies
          ** from getting re-ordered.
          *******************************************************************/
          compilerBarrier();
          _readIndex.store(rindex, std::memory_order_relaxed);
        }

        /******************************************************************
        ** prevent future loads from getting reordered before this load
        ** loading data from buffer, has to strictly happen after the store
        ** to writeIndex  is visible
        ******************************************************************/
        auto windex = _writeIndex.load(std::memory_order_acquire);

        /********************************************************************
        ** rindex + 1 will never overflow - why ?
        ** rindex is supposed to follow windex. sematically rindex should
        ** always be less than windex.
        **
        ** Invariant #1 - the value of rindex will always be less than windex
        **                and windex will never overflow.
        ** Invariant #2 - At anytime, windex cannot be greater than 
        **                (rindex + poolSize).
        ** Invariant #3 - poolSize > 1
        ** Deducing from the above three invariants, rindex + 1 cannot overflow.
        ********************************************************************/
        if(windex > rindex + 1) {
          return bufferAt(rindex+1);
        }
        return nullptr;
      }

      uint64_t writeIndex() const noexcept {
        return _writeIndex.load(std::memory_order_relaxed);
      }

      uint64_t readIndex() const noexcept {
        return _readIndex.load(std::memory_order_relaxed);
      }

      uint64_t overflowCount() const noexcept {
        return _overflowCount;
      }

      /*******************************************************************
      ** This method has a RACE between writer and reader thread
      *******************************************************************/
      const T* peekWithDataRace() const noexcept {
        auto windex = _writeIndex.load(std::memory_order_relaxed);
        return bufferAt(windex);
      }

    private:

      const T* bufferAt(uint64_t index_) const noexcept {
        return const_cast<WaitFreeBufferPool*>(this)->bufferAt(index_);
      }

      T* bufferAt(uint64_t index_) noexcept {
        auto bufferIndex = (index_  & poolSizeMask) * bufferSize;
        return &_pool->data()[bufferIndex];
      }

      // pack all index/count in one cache line
      volatile std::atomic<uint64_t> _writeIndex;
      volatile std::atomic<uint64_t> _readIndex;
      const std::unique_ptr<Pool> _pool;
      volatile uint64_t _overflowCount;
      static constexpr size_t dataSize = sizeof(_writeIndex) + sizeof(_readIndex) + sizeof(_pool) + sizeof(_overflowCount);
      const char _[ALIGNMENT - dataSize]; // padding

      // compile time const expressions
      static constexpr uint64_t poolSizeMask = poolSize -1;
      static constexpr uint64_t readIndexMax = std::numeric_limits<uint64_t>::max() - poolSize;
      static_assert(dataSize + sizeof(_) == ALIGNMENT, "object expected to occupy one cache line");
  };

}}
