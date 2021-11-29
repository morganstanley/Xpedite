///////////////////////////////////////////////////////////////////////////////
//
// Socket - A tcp socket with support for non-blocking read and write operations
//
// The class also provides methods that retry a read or write, till
// the operation is completed or an error occurs
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Platform.H"
#include <xpedite/log/Log.H>
#include <xpedite/util/Errno.H>
#include <sstream>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

namespace xpedite { namespace transport { namespace tcp {

  class Listener;

  inline sockaddr* addr_cast(sockaddr_in* addr_) {
    return reinterpret_cast<sockaddr*>(addr_);
  }

  inline sockaddr* addr_cast(sockaddr_in& addr_) {
    return addr_cast(&addr_);
  }

  class Socket
  {
    friend class Listener;

    Socket()
      : _fd {-1}, _addr {}, _eof {}, _blocking {true} {
    }

    public:

      Socket(std::string ipAddr_, int port_)
        : _fd {-1}, _addr {AF_INET, htons(port_), {INADDR_ANY}, {}}, _eof {}, _blocking {true} {
        if(inet_pton(AF_INET, ipAddr_.c_str(), &_addr.sin_addr) <=0) {
          auto errMsg = "Invalid argument - IP Address " + ipAddr_ + " is not valid";
          XpediteLogCritical << toString() << " failed to construct. " << errMsg << XpediteLogEnd;
          throw std::invalid_argument {errMsg};
        }
      }

      // cannot be copied
      Socket(const Socket&) = delete;
      Socket& operator=(const Socket&) = delete;
      Socket& operator=(Socket&&) = delete;

      Socket(Socket&& other_)
        : _fd {other_._fd}, _addr (other_._addr), _eof {other_._eof}, _blocking {other_._blocking} {
        other_._fd = -1;
        other_._eof = {};
      }

      ~Socket() {
        cleanup();
      }

      const sockaddr_in& addr() const noexcept { return _addr; }
      int fd()                  const noexcept { return _fd;   }
      bool eof()                const noexcept { return _eof;  }

      void handleError(const char* action_) noexcept {
        xpedite::util::Errno e;
        XpediteLogCritical << toString() << " " << action_ << " - " << e.asString() << XpediteLogEnd;
        cleanup();
      }

      bool setNoDelay() noexcept {
        int flag = 1;
        int result = setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&flag), sizeof(flag));
        if(result < 0) {
          handleError("failed to switch off nagle for socket");
          return false;
        }
        return true;
      }

      bool setNonBlocking() noexcept {
        auto flags = fcntl(_fd, F_GETFL);
        if(flags != -1) {
          flags = fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
        }

        if(flags == -1) {
          handleError("failed to set Non blocking mode");
          return false;
        }
        _blocking = false;
        return true;
      }

      bool connect() noexcept {
        _fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if(_fd < 0) {
          xpedite::util::Errno e;
          XpediteLogCritical << toString() << " failed to create socket - " << e.asString() << XpediteLogEnd;
          return false;
        }

        if(!setNoDelay()) {
          return false;
        }

        XpediteLogInfo << toString() << " connecting to endpoint" << XpediteLogEnd;
        if(::connect(_fd, addr_cast(_addr), sizeof(_addr))) {
          handleError("failed to connect to endpoint");
          return false;
        }
        return true;
      }

      void cleanup() noexcept {
        if(_fd >=0) {
          if(close(_fd) < 0) {
            xpedite::util::Errno e;
            XpediteLogCritical << toString() << " failed to close socket " << e.asString() << XpediteLogEnd;
          }
        }
        _fd = -1;
        _eof = {};
      }

      int read(char* buffer_, int len_) noexcept {
        auto dataLen = ::read(_fd, buffer_, len_);
        if(dataLen <= 0) {
          _eof = true;
          if(dataLen < 0) {
            xpedite::util::Errno e;
            XpediteLogCritical << "Socket [" << _fd << "] failed to read data" << e.asString() << XpediteLogEnd;
          }
          return 0;
        }
        return dataLen;
      }

      int write(const char* buffer_, int len_) noexcept {
        auto expectedWriteLen = len_;
        do {
          auto written = ::write(_fd, buffer_, len_);
          if(written <= 0) {
            if(written == 0 || (errno != EAGAIN && errno != EINTR)) {
              _eof = true;
              xpedite::util::Errno e;
              XpediteLogCritical << "Socket [" << _fd << "] failed to write data" << e.asString() << XpediteLogEnd;
              break;
            }
            written = 0;
          }
          len_ -= written;
          buffer_ += written;
        } while(len_ > 0);
        return expectedWriteLen - len_;
      }

      std::string toString() noexcept {
        std::array<char, 256> buffer {};
        if(!inet_ntop(AF_INET, &_addr.sin_addr, buffer.data(), buffer.size())) {
          // error converting ip to string
          strcpy(buffer.data(), "unknown");
        }
        std::ostringstream stream;
        stream << "ip - " << buffer.data() << " | port - " << ntohs(_addr.sin_port) << " | fd - " << _fd
               << " | mode - " << (_blocking ? "Blocking" : "NON-Blocking");
        return stream.str();
      }

    private:

      int _fd;
      sockaddr_in _addr;
      bool _eof;
      bool _blocking;
  };

}}}
