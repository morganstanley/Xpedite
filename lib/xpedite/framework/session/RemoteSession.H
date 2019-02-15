////////////////////////////////////////////////////////////////////////////////////////////////
//
// RemoteSession - Manages sessions from external profiler instance.
//
// The remote session listens to a non-blocking socket to accept tcp connections from profiler.
//
// The logic ensures that, no more than one client connection, can be active at a time.
// Any attempts to establish a new connection, during active sessions are rejected.
//
// Disconnection of the profiler tcp connection will automatically restore state by
// disabling probes and pmc that were activated during the session.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/transport/Listener.H>
#include <xpedite/transport/Framer.H>
#include <xpedite/log/Log.H>
#include "../request/RequestParser.H"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace xpedite { namespace framework { namespace session {

  constexpr bool isListenerBlocking = false;
  constexpr int RC_SUCCESS = 0;
  constexpr int RC_FAILURE = 1;

  class RemoteSession
  {
    Handler& _handler;
    xpedite::transport::tcp::Listener _listener;
    std::unique_ptr<transport::tcp::Socket> _client;
    xpedite::transport::tcp::Framer _framer;
    request::RequestParser _parser;

    void pollClient() noexcept;

    void resetClient() {
      if(_handler.isProfileActive()) {
        _handler.endProfile();
      }
      _client.reset();
      _framer.reset();
    }

    static std::string encode(int returnCode_, const std::string& payload) {
      std::ostringstream stream;
      stream << std::setfill('0') << std::setw(8) << payload.size() + 5
             << "rc=" << returnCode_ << "|" << std::setfill(' ') << payload;
      return stream.str();
    }


    static std::string encode(request::RequestPtr& request_) {
      const auto& response = request_->response();
      auto returnCode = response ? RC_SUCCESS : RC_FAILURE;
      auto payload = response ? response.value() : response.errors();
      return encode(returnCode, payload);
    }

    request::RequestPtr parseFrame(xpedite::transport::tcp::Frame frame_) noexcept {
      XpediteLogDebug << "rx frame (" << frame_.size() << " bytes) - " 
        <<  std::string {frame_.data(), static_cast<std::size_t>(frame_.size())} << XpediteLogEnd;
      return _parser.parse(frame_.data(), frame_.size());
    }

    public:

    RemoteSession(Handler& handler_, std::string listenerIp_, in_port_t port_)
      : _handler(handler_), _listener {"xpedite", isListenerBlocking, listenerIp_, port_},
        _client {}, _framer {}, _parser {} {
    }

    void start() {
      if(!_listener.start()) {
        std::ostringstream stream;
        stream << "xpedite framework init error - Failed to start listener " << _listener.toString();
        throw std::runtime_error {stream.str()};
      }
    }

    in_port_t listenerPort() const noexcept {
      return _listener.port();
    }

    bool isAlive() const noexcept {
      return static_cast<bool>(_client);
    }

    bool poll(bool canAcceptRequest_);

    void shutdown() {
      if(isAlive()) {
        XpediteLogCritical << "xpedite - remote session - framework is going down." << XpediteLogEnd;
        resetClient();
      }
    }
  };

}}}
