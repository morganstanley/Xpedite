////////////////////////////////////////////////////////////////////////////////////////////////
//
// SessionManager - manages life cycle of profiling sessions.
//
// The manager supports two types of sessions
//  1. Local session - Initiated from within process context.
//  2. Remote session - Initiated from profiler exteranl to the application.
//
// The manager keeps track of current session state and ensures no more than one session
// is active at a time.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "RemoteSession.H"
#include "LocalSession.H"
#include <memory>
#include <cassert>

namespace xpedite { namespace framework { namespace session {

  class SessionManager
  {
    enum SessionType {
      DORMANT,
      REMOTE,
      LOCAL
    };

    Handler _handler;
    std::unique_ptr<LocalSession> _localSession;
    RemoteSession _remoteSession;
    SessionType _sessionType;

    public:

    SessionManager(std::string listenerIp_, in_port_t port_)
      : _handler {}, _localSession {new LocalSession {_handler}},
        _remoteSession {_handler, listenerIp_, port_}, _sessionType {DORMANT} {
    }

    void start() {
      _localSession->start();
      _remoteSession.start();
    }

    in_port_t listenerPort() const noexcept {
      return _remoteSession.listenerPort();
    }

    bool isProfileActive() const noexcept {
      return _handler.isProfileActive();
    }

    MilliSeconds pollInterval() const noexcept {
      return _sessionType == DORMANT ? MilliSeconds {500}: _handler.pollInterval();
    }

    void poll() {
      if(_localSession->poll(_sessionType == DORMANT || _sessionType == LOCAL)) {
        assert(_sessionType == DORMANT || _sessionType == LOCAL);
        _sessionType = LOCAL;
      } else if(_sessionType == LOCAL) {
        _sessionType = DORMANT;
      }

      if(_remoteSession.poll(_sessionType == DORMANT || _sessionType == REMOTE)) {
        assert(_sessionType == DORMANT || _sessionType == REMOTE);
        _sessionType = REMOTE;
      } else if(_sessionType == REMOTE) {
        _sessionType = DORMANT;
      }

      if(_sessionType != DORMANT) {
        _handler.poll();
      }
    }

    bool execute(request::Request* request_) {
      //hardcoded to no timilimit, as requests are allocated in stack
      MilliSeconds timeout= MilliSeconds {0};
      return _localSession->execute(request_, timeout);
    }

    void shutdown() {
      _remoteSession.shutdown();
      _localSession->shutdown();
      _handler.shutdown();
    }
  };

}}}
