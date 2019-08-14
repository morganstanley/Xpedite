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
#include <xpedite/common/LeasedPtr.H>
#include <xpedite/util/Allocator.H>
#include <xpedite/log/Log.H>
#include <sstream>
#include <stdexcept>
#include <atomic>

namespace xpedite { namespace framework { namespace session {

  class LocalSession : public util::AlignedObject<XPEDITE_CACHELINE_SIZE>
  {
    common::LeasedPtr<request::Request> _request;

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
      auto request = request_->clone();
      // enque request
      while(!hasTimedOut(timeout_, elapsed)) {
	request = std::move(_request.provision(std::move(request)));
	if (!request) {
	  break;
	}

        std::this_thread::sleep_for(_handler.pollInterval());
        elapsed += _handler.pollInterval();
      }
      
      // await execution
      while(!hasTimedOut(timeout_, elapsed) && !_request.empty()) {
        std::this_thread::sleep_for(_handler.pollInterval());
        elapsed += _handler.pollInterval();
      }

      const auto releasedRequest = _request.revoke();
      if (releasedRequest) {
	request_->response() = std::move(request->response());
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
      if(auto request = _request.lease()) {
        if(canAcceptRequest_) {
          request->execute(_handler);
          _isAlive = true;
        } else {
          request->abort("xpedite dectected active session - multiple sessions not supported");
        }
	_request.returnLease(std::move(request));
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
