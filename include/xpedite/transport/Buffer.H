///////////////////////////////////////////////////////////////////////////////
//
// A circular buffer to store and consume data from tcp sockets
//
// The buffer keeps track of available storage, moves data or expands capacity
// as needed, to accomodate storage requirements
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/platform/Builtins.H>
#include <xpedite/log/Log.H>
#include <errno.h>
#include <stdexcept>
#include <unistd.h>
#include <string.h>
#include <xpedite/util/Errno.H>

namespace xpedite { namespace transport { namespace util {

struct Buffer {

  explicit Buffer(uint32_t capacity_) 
    : _capacity {capacity_}, _in {}, _out {} {
    _buffer = static_cast<char*>(::malloc(capacity_));
    ::bzero(_buffer, capacity_);
  }

  ~Buffer() {
    ::free(_buffer);
  }

  void reset() {
    _in = {};
    _out = {};
    ::bzero(_buffer, _capacity);
  }

  uint32_t capacity() const {
    return _capacity;
  }

  uint32_t usableCapacity() const {
    return _capacity - _in;
  }

  uint32_t size() const {
    return _in - _out;
  }

  char* getReadBuffer() const {
    return _buffer + _out;
  }

  void advanceRead(uint32_t size_) {
    if(XPEDITE_UNLIKELY(_out + size_ > _in)) {
      throw std::runtime_error {"detected attempt to read past available data in buffer"};
    }
    _out += size_;
  }

  void advanceReadUnsafe(uint32_t size_) {
    _out += size_;
  }

  int read(int fd_) {
    if(XPEDITE_UNLIKELY(usableCapacity() == 0)) {
      XpediteLogDebug << "failed to read. not enough usable capacity available" << XpediteLogEnd;
      return 0;
    }
    int status = ::read(fd_, _buffer + _in, usableCapacity());
    if(XPEDITE_UNLIKELY(status == -1)) {
      int err = errno;
      switch (err) {
        case EAGAIN:
        case EINTR:
          return 0;
        default:
          xpedite::util::Errno e;
          XpediteLogDebug << "socket read for fd (" << fd_ << ") failed - " << e.asString() << XpediteLogEnd;
          return -1;
      }
    }

    if(XPEDITE_UNLIKELY(status == 0)) {
      XpediteLogDebug << "socket read for fd (" << fd_ << ") terminated - recieved EOF." << XpediteLogEnd;
      return -1;
    }
    _in += status;
    return status;
  }

  void ensureRoom(uint32_t size_) {
    if(XPEDITE_UNLIKELY(_in + size_ > _capacity)) {
      int32_t needed = size() + size_ - _capacity;
      if(needed > 0) {
        uint32_t newSize = _capacity;
        while (newSize < _capacity + needed) {
          newSize *= 2;
        }
        XpediteLogDebug << "reallocating to new size - " << newSize << XpediteLogEnd;

        char* buffer = static_cast<char*>(realloc(_buffer, newSize));
        if(nullptr == buffer) {
          throw std::bad_alloc();
        }
        _buffer = buffer;
        _capacity = newSize;
      }
      else if(XPEDITE_UNLIKELY(size() > 0)) {
        XpediteLogDebug << "relocating data (" << size() << " bytes)"  << XpediteLogEnd;
        ::memmove(static_cast<void*>(_buffer), static_cast<void*>(_buffer + _out), size());
      }
      _in -= _out;
      _out = 0;
    }
  }

  private:

  char* _buffer;
  uint32_t _capacity;
  uint32_t _in;
  uint32_t _out;
};

}}}
