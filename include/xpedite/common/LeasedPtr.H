//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LeasedPtr - a SPSC wait free data structure for sharing a resource between two threads.
//
// It has a clear ownership semantics, allowing the consumer to lease the pointer from the producer.
//
// Once the consumer has successfully borrowed the resource, the producer will know not to deallocate
// the resource until the consumer has returned the lease.
//
// If the consumer takes too long to return the resource, the producer can release the ownership.
// The LeasedPtr will ensure the consumer knows it now owns the resource and is responsible for deallocation.
//
// Author: Marcin Dlugajczyk, Morgan Stanley
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <atomic>
#include <cassert>
#include <memory>

namespace xpedite { namespace common {

  template<typename T>
  class LeasedPtr
  {
    // The LeasedPtr can be in one of 5 states.
    // Dormant - the class is empty, and is ready to accept `provision` request from the producer.
    // Provisioned - producer has stored a pointer, and it hasn't been leased by the consumer yet.
    // Leased - the consumer has leased the ptr.
    // Revoked - the producer has relenquished the ownership of the pointer.
    // Returned - the consumer has returned the leased ptr.
    enum class State {
      Dormant,
      Provisioned,
      Leased,
      Revoked,
      Returned
    };
    std::unique_ptr<T> _ptr{nullptr};
    std::atomic<State> _state{State::Dormant};

  public:
    LeasedPtr() noexcept = default;
    explicit LeasedPtr(T *ptr_) noexcept : _ptr{ptr_} {}
    ~LeasedPtr() = default;
    LeasedPtr<T>(LeasedPtr<T> &other) = delete;
    LeasedPtr<T>(LeasedPtr<T> &&other) = delete;
    LeasedPtr<T>& operator=(LeasedPtr<T> &other) = delete;
    LeasedPtr<T>& operator=(LeasedPtr<T> &&other) = delete;

    // Tries to deposit the `ptr_`, and make it available to the consumer.
    // Returns empty std::unique_ptr if the pointer was susscessfully deposited.
    // Returns the original ptr otherwise.
    std::unique_ptr<T> provision(std::unique_ptr<T> ptr_) noexcept {
      if (!empty()) {
	// LeasedPtr holds a different value. Can't store a new pointer.
	return ptr_;
      }

      // LeasedPtr is empty and ready to be provisioned.
      _ptr = std::move(ptr_);
      _state.store(State::Provisioned, std::memory_order_release);
      return {};
    }

    // Indicates if has space for new deposit.
    bool empty() const noexcept {
      return _state.load(std::memory_order_relaxed) == State::Dormant;
    }

    // Producer breaks the lease, and releases the ownership of the pointer and passes it to the consumer.
    // Returns true on success. Returns false if the consumer hasn't leased the pointer yet - the ownership stays with the producer.
    std::unique_ptr<T> revoke() noexcept {
      auto state{State::Provisioned};
      if (_state.compare_exchange_strong(state, State::Dormant, std::memory_order_release, std::memory_order_acquire)) {
	return std::move(_ptr);
      }

      assert ((state == State::Leased) || (state == State::Returned));

      // Someone has borrowed the pointer, we're trying to abandon it
      if (state == State::Leased && _state.compare_exchange_strong(state, State::Revoked, std::memory_order_release, std::memory_order_acquire)) {
	return {};
      }

      if (state == State::Returned && _state.compare_exchange_strong(state, State::Dormant, std::memory_order_release, std::memory_order_acquire)) {
	return std::move(_ptr);
      }

      // Should not reach.
      assert(false);
      return {};
    }

    // Attempts to lease the pointer. On success returns the deposited pointer.
    // Returns nullptr on failure.
    std::unique_ptr<T> lease() noexcept {
      auto provisioned{State::Provisioned};
      auto successfullyLeased = _state.compare_exchange_strong(provisioned, State::Leased,
							       std::memory_order_release,
							       std::memory_order_acquire);

      return successfullyLeased ? std::move(_ptr) : std::unique_ptr<T>{};
    }

    // The consumer returns the pointer back to the producer. If the operation was sucessful, an empty unique_ptr is retruned.
    // Otherwise, the original pointer is returned.
    std::unique_ptr<T> returnLease(std::unique_ptr<T> ptr_) noexcept {
      assert((_state.load(std::memory_order_relaxed) == State::Leased) || _state.load(std::memory_order_relaxed) == State::Revoked);

      _ptr = std::move(ptr_);
      auto leased{State::Leased};
      if (_state.compare_exchange_strong(leased, State::Returned, std::memory_order_release, std::memory_order_acquire)) {
	return {};
      }

      assert(_state.load(std::memory_order_relaxed) == State::Revoked);
      ptr_ = std::move(_ptr);
      _state.store(State::Dormant, std::memory_order_release);
      return ptr_;
    }
  };
}}
