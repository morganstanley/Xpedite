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

#include "RemoteSession.H"

namespace xpedite { namespace framework { namespace session {

  bool RemoteSession::poll(bool canAcceptRequest_) {
    if(auto clientSocket = _listener.accept()) {
      if(!canAcceptRequest_ || _client) {
        std::string pdu = encode(RC_FAILURE, "xpedite dectected active session - multiple sessions not supported");
        clientSocket->write(pdu.data(), pdu.size());
      } else {
        XpediteLogInfo << "xpedite - accepted incoming connection from " << clientSocket->toString() << XpediteLogEnd;
        _client = std::move(clientSocket);
        _framer.reset(_client.get());
      }
    }
    if(canAcceptRequest_ && _client) {
      pollClient();
    }
    return isAlive();
  }

  void RemoteSession::pollClient() noexcept {
    try {
      while(auto frame = _framer.readFrame()) {
        auto request = parseFrame(frame);
        request->execute(_handler);
        XpediteLogInfo << "exec request - " << request->toString() << XpediteLogEnd;
        std::string pdu = encode(request);
        if(_client->write(pdu.data(), pdu.size()) != static_cast<int>(pdu.size())) {
          XpediteLogCritical << "xpedite - handler error, failed to send result " 
            << pdu << " to client " << _client->toString() << XpediteLogEnd;
          resetClient();
          break;
        }
      }
      return;
    }
    catch(std::runtime_error& e_) {
      XpediteLogCritical << "xpedite - closing client connection - error " << e_.what() << XpediteLogEnd;
    }
    catch(...) {
      XpediteLogCritical << "xpedite - closing client connection - unknown error" << XpediteLogEnd;
    }
    resetClient();
  }

}}}
