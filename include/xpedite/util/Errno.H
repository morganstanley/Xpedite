///////////////////////////////////////////////////////////////////////////////
//
// Errno - utility class to build human friendly error messages
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstring>
#include <array>
#include <cerrno>

namespace xpedite { namespace util {

  struct Errno
  {
    int lastErrorCode() const {
      return errno;
    }

    const char* asString() const {
      return strerror_r(lastErrorCode(), _buffer.data(), _buffer.size());
    }

    private:

    // The size of buffer is picked based on the size used by glibc strerror(...)
    mutable std::array<char, 1024> _buffer; 
  };

}}
