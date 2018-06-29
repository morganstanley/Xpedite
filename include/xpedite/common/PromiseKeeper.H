///////////////////////////////////////////////////////////////////////////////
//
// PromiseKeeper - A utility class that uses RAII to deliver on a promise
// on scope exit
// 
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <future>
#include <exception>

namespace xpedite { namespace common {

  template<typename T>
  struct PromiseKeeper
  {
    PromiseKeeper(std::promise<T>* promise_)
      : _promise (promise_) {
    }

    PromiseKeeper(const PromiseKeeper&) = delete;
    PromiseKeeper& operator=(const PromiseKeeper&) = delete;
    PromiseKeeper(PromiseKeeper&&) = delete;
    PromiseKeeper& operator=(PromiseKeeper&&) = delete;

    ~PromiseKeeper() {
      if(_promise && !std::uncaught_exception()) {
        deliver(T {});
      }
    }

    void deliver(T&& result_) {
      if(_promise) {
        _promise->set_value(std::forward<T>(result_));
        _promise = nullptr;
      }
    }

    bool isPending() const noexcept {
      return _promise;
    }

    private:
    std::promise<T>* _promise;
  };
}}
