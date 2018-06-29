///////////////////////////////////////////////////////////////////////////////
//
// A utility class to emulate dynamic allocation of thread local data 
// in a finite size global static container
// 
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <array>
#include <atomic>
#include <cassert>
#include <xpedite/util/Util.H>

namespace xpedite { namespace intercept {

  template <typename T, size_t MaxSize>
  class TlScopedDatumPtr;

  template <typename T, size_t MaxSize>
  struct TlScopedData;

  template <typename T, size_t MaxSize>
  class TlScopedDatum
  {
    friend class std::array<TlScopedDatum, MaxSize>;
    friend class TlScopedDatumPtr<T, MaxSize>;

    std::atomic<pid_t> _tid;

    T _data;

    int _useCount;

    TlScopedDatum()
      : _tid {}, _data {}, _useCount {} {
    }

    public:

    T& data() {
      return _data;
    }

    bool isUsed(pid_t tid_) noexcept {
      return _tid.load(std::memory_order_relaxed) == tid_;
    }

    bool tryUse(pid_t tid_) noexcept {
      pid_t expected = 0;
      return _tid.compare_exchange_strong(expected, tid_, std::memory_order_relaxed, std::memory_order_relaxed);
    }

    void acquire() {
      assert(this->_useCount >=0);
      assert(this->_tid.load(std::memory_order_relaxed) != util::gettid());
      ++_useCount;
    }

    void release() {
      assert(this->_useCount > 0);
      if(--_useCount == 0) {
        _tid.store(0, std::memory_order_relaxed);
      }
    }
  };

  template <typename T, size_t MaxSize>
  class TlScopedDatumPtr
  {
    friend class TlScopedData<T, MaxSize>;

    using TlScopedDatumType = TlScopedDatum<T, MaxSize>;

    TlScopedDatumType* _tlScopedDatum;

    explicit TlScopedDatumPtr(TlScopedDatumType* tlScopedDatum_) noexcept
      : _tlScopedDatum {tlScopedDatum_} {
      _tlScopedDatum->acquire();
    }

    public:

    TlScopedDatumPtr(const TlScopedDatumPtr&)            = delete;
    TlScopedDatumPtr& operator=(const TlScopedDatumPtr&) = delete;
    TlScopedDatumPtr& operator=(TlScopedDatumPtr&&)      = delete;

    TlScopedDatumPtr(TlScopedDatumPtr&& other_)
      : _tlScopedDatum {other_._tlScopedDatum} {
      other_._tlScopedDatum = nullptr;
    }

    ~TlScopedDatumPtr() {
      if(_tlScopedDatum) {
        _tlScopedDatum->release();
      }
    }

    int useCount() const noexcept {
      return _tlScopedDatum ? _tlScopedDatum->_useCount : 0;
  
    }

    T* operator->() noexcept {
      return &_tlScopedDatum->_data;
    }

    explicit operator bool() {
      return _tlScopedDatum != nullptr;
    }
  };

  template <typename T, size_t MaxSize>
  struct TlScopedData
  {
    static std::array<TlScopedDatum<T, MaxSize>, MaxSize> _tlScopedData;
    static TlScopedDatumPtr<T, MaxSize> get();
  };

  template <typename T, size_t MaxSize>
  std::array<TlScopedDatum<T, MaxSize>, MaxSize> TlScopedData<T, MaxSize>::_tlScopedData;
  
  template <typename T, size_t MaxSize>
  TlScopedDatumPtr<T, MaxSize> TlScopedData<T, MaxSize>::get() {
    auto curTid = util::gettid();
    for(auto& tlDatum : _tlScopedData) {
      if(tlDatum.isUsed(curTid)) {
        return TlScopedDatumPtr<T, MaxSize> {&tlDatum};
      }
    }

    for(auto& tlDatum : _tlScopedData) {
      if(tlDatum.tryUse(curTid)) {
        return TlScopedDatumPtr<T, MaxSize> {&tlDatum};
      }
    }
    return TlScopedDatumPtr<T, MaxSize> {nullptr};
  }

}}
