///////////////////////////////////////////////////////////////////////////////
//
// Frame - an abstraction used in framing messages out of a data stream
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

namespace xpedite { namespace transport { namespace tcp {
  
  struct Frame
  {
    Frame()
      : _data {}, _size {} {
    }

    Frame(char* data_, size_t size_)
      : _data {data_}, _size {size_} {
    }

    char* data() noexcept  { return _data; }
    size_t size() noexcept { return _size; }

    explicit operator bool() const noexcept {
      return _size > 0;
    }

    private:

    char* _data;
    size_t _size;
  };

}}}
