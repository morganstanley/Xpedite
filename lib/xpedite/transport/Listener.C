///////////////////////////////////////////////////////////////////////////////
//
// Listener - a tcp listener with support for non-blocking sockets
//
// The listener has logic to accept and configure tcp connections.
// A non blocking listener will make all connections non blocking.
// Nagle is disabled for all accepted connections.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/transport/Listener.H>
#include <xpedite/log/Log.H>
#include <arpa/inet.h>
#include <sstream>
#include <stdexcept>

namespace xpedite { namespace transport { namespace tcp {

  Listener::Listener(std::string name_, bool blocking_, std::string address_, in_port_t port_)
    : _name {std::move(name_)}, _fd {platform::invalidFileDescriptor}, _addrStr {std::move(address_)},
      _addr {AF_INET, htons(port_), {INADDR_ANY}, {}}, _errno {}, _blocking {blocking_} {
    if(_addrStr.size()) {
      if(inet_pton(AF_INET, _addrStr.c_str(), &_addr.sin_addr) <=0) {
        auto errMsg = "Invalid argument - IP Address " + _addrStr + " is not valid";
        XpediteLogCritical << toString() << " failed to construct. " << errMsg << XpediteLogEnd;
        throw std::invalid_argument {errMsg};
      }
    }
  }

  Listener::~Listener() {
    if(*this) {
      XpediteLogCritical << toString() << " is being destroyed while active." << XpediteLogEnd;
      stop();
    }
  }

  bool Listener::start() noexcept {
    _fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(_fd < 0) {
      XpediteLogCritical << toString() << " failed to create socket - " << _errno.asString() << XpediteLogEnd;
      return false;
    }

    if(!_blocking) {
      auto flags = fcntl(_fd, F_GETFL);
      if(flags != -1) {
        flags = fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
      }

      if(flags == -1) {
        XpediteLogCritical << toString() << " failed to enable non-blocking mode for listener fd [" << _fd << "] " << XpediteLogEnd;
        return false;
      }
    }

    XpediteLogInfo << toString() << " binding to port " << _addr.sin_port << XpediteLogEnd;
    if(bind(_fd, addr_cast(_addr), sizeof(_addr))) {
      XpediteLogCritical << toString() << " failed to bind socket fd [" << _fd << "] to ip address " 
        << _addrStr << " - " << _errno.asString() << XpediteLogEnd;
      return false;
    }

    if(listen(_fd, 4096 /* backlog */)) {
      XpediteLogCritical << toString() << " failed to listen on socket fd [" << _fd << "] - " << _errno.asString() << XpediteLogEnd;
      return false;
    }

    if(!port()) {
      socklen_t len = sizeof(_addr);
      if(getsockname(_fd, addr_cast(_addr), &len)) {
        XpediteLogCritical << toString() << " failed to get port allocation for listen socket fd [" << _fd << "] - " << _errno.asString() << XpediteLogEnd;
        return false;
      }
    }
    XpediteLogInfo << toString() << " listening for incoming connections " << port() << XpediteLogEnd;
    return true;
  }

  bool Listener::stop() noexcept {
    if(*this) {
      close(_fd);
    }
    return true;
  }

  std::tuple<Listener::AcceptState, std::unique_ptr<Socket>> Listener::_accept() noexcept {
    Socket socket;
    socklen_t addrSize = sizeof(socket._addr);
    socket._fd = ::accept(_fd, addr_cast(socket._addr), &addrSize);
    if(socket._fd < 0) {
			int err = errno;
			switch (err) {
        case EAGAIN:
        case EINTR:
          return std::make_tuple(AcceptState::Await, std::unique_ptr<Socket> {});
        default:
          return std::make_tuple(AcceptState::ErrorDetected, std::unique_ptr<Socket> {});
      }
    }
    return std::make_tuple(AcceptState::Accepted, std::unique_ptr<Socket> {new Socket{std::move(socket)}});
  }

  std::unique_ptr<Socket> Listener::accept() {
    AcceptState acceptState;
    std::unique_ptr<Socket> socket;
    std::tie(acceptState, socket) = _accept();
    if(acceptState == AcceptState::ErrorDetected) {
      std::ostringstream stream;
      stream << toString() << " failed to accept incoming connection " << _errno.asString();
      throw std::runtime_error {stream.str()};
    }

    if(!socket) {
      return socket;
    }

    if(!_blocking && !socket->setNonBlocking()) {
      std::ostringstream stream;
      stream << toString() << " failed to enable Non blocking mode for socket " << socket->toString() << " - " << _errno.asString();
      throw std::runtime_error {stream.str()};
    }

    if(!socket->setNoDelay()) {
      std::ostringstream stream;
      stream << toString() << " failed to disable nagle for socket " << socket->toString() << " - " << _errno.asString();
      throw std::runtime_error {stream.str()};
    }
    return socket;
  }


  // should not be called in crit path
  std::string Listener::toString() const {
    std::array<char, 256> buffer {};
    if(!inet_ntop(AF_INET, &_addr.sin_addr, buffer.data(), buffer.size())) {
      // error converting ip to string
      strcpy(buffer.data(), "unknown");
    }

    std::ostringstream stream;
    stream << "Listener " << _name << " [fd - " << _fd << " | ip - " << buffer.data() << " | port - " 
           << ntohs(_addr.sin_port) << " | mode - " << (_blocking ? "Blocking" : "NON-Blocking");
    return stream.str();
  }

}}}
