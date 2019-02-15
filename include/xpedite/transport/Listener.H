///////////////////////////////////////////////////////////////////////////////
//
// Listener - a tcp listener with support for non-blocking sockets
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Platform.H"
#include "Socket.H"
#include <arpa/inet.h>
#include <xpedite/util/Errno.H>
#include <string>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <memory>
#include <tuple>

namespace xpedite { namespace transport { namespace tcp {

  class Listener
  {
    public:

      Listener(std::string name_, bool blocking_, std::string address_ = {}, in_port_t port_ = {});
      Listener(const Listener&) = delete;             // cannot be copied
      Listener& operator=(const Listener&) = delete;  // cannot be copied
      Listener& operator=(Listener&&) = delete;       // prevent active sockets from being mutated

      // can be moved
      Listener(Listener&& other_)
        : _fd {other_._fd} {
        other_._fd = platform::invalidFileDescriptor; 
      }

      ~Listener();

      explicit operator bool() const {
        return _fd != platform::invalidFileDescriptor;
      }

      int socket() const { return _fd;                   }
      int port()   const { return ntohs(_addr.sin_port); }

      void setBlocking()    { _blocking = true;  }
      void setNonBlocking() { _blocking = false; }

      bool start() noexcept;
      bool stop() noexcept;
      std::unique_ptr<Socket> accept();

      // should not be called in crit path
      std::string toString() const;

    private:

      enum class AcceptState
      {
        Accepted, ErrorDetected, Await
      };

      std::tuple<AcceptState, std::unique_ptr<Socket>> _accept() noexcept;

      std::string _name;
      int _fd;
      std::string _addrStr;
      sockaddr_in _addr;
      xpedite::util::Errno _errno;
      bool _blocking;
  };
}}}
