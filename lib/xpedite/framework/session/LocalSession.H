////////////////////////////////////////////////////////////////////////////////////////////////
//
// LocalSession - manages profiling session from local process context.
//
// The local session supports execution of requests from other threads in the process.
// Requests are enqueued one at a time and the calling thread is blocked till the 
// execution completes or times out.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/util/Allocator.H>
#include <xpedite/log/Log.H>
#include <sstream>
#include <stdexcept>
#include <atomic>

namespace xpedite { namespace framework { namespace session {

  class LocalSession : public util::AlignedObject<XPEDITE_CACHELINE_SIZE>
  {
    std::atomic<request::Request*> _request;

    alignas(XPEDITE_CACHELINE_SIZE) Handler& _handler;

    bool _isAlive;

    static bool hasTimedOut(MilliSeconds duration_, MilliSeconds elapsed_) {
      if(duration_ == MilliSeconds {0}) {
        return {};
      }
      return elapsed_ > duration_;
    }

    public:

    LocalSession(Handler& handler_)
      : _request {}, _handler (handler_), _isAlive {} {
    }

    bool execute(request::Request* request_, MilliSeconds timeout_) {
      MilliSeconds elapsed {0};
      // enque request
      while(!hasTimedOut(timeout_, elapsed)) {
        request::Request* expected = {};
        if(_request.compare_exchange_weak(expected, request_, std::memory_order_release, std::memory_order_relaxed)) {
          break;
        }
        std::this_thread::sleep_for(_handler.pollInterval());
        elapsed += _handler.pollInterval();
      }
      
      // await execution
      while(!hasTimedOut(timeout_, elapsed) && _request.load(std::memory_order_acquire) == request_) {
        std::this_thread::sleep_for(_handler.pollInterval());
        elapsed += _handler.pollInterval();
      }

      if(hasTimedOut(timeout_, elapsed)) {
        request_->abort("timed out");
        return {};
      }
      return static_cast<bool>(request_->response());
    }

    void start() {
    }

    bool isAlive() const noexcept {
      return _isAlive;
    }

    bool poll(bool canAcceptRequest_) {
      if(auto* request = _request.load(std::memory_order_acquire)) {
        if(canAcceptRequest_) {
          request->execute(_handler);
          _isAlive = true;
        } else {
          request->abort("xpedite dectected active session - multiple sessions not supported");
        }
        _request.store(nullptr, std::memory_order_release);
      }
      return isAlive();
    }

    void shutdown() {
      if(isAlive()) {
        XpediteLogCritical << "xpedite - local session - framework is going down." << XpediteLogEnd;
      }
    }
  };

}}}
