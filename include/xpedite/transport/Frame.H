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
      : _data {}, _size {}, _isEOF{0} {
    }

    Frame(char* data_, size_t size_)
      : _data {data_}, _size {size_}, _isEOF{0} {
    }

    Frame(char* data_, size_t size_, bool isEOF_)
      : _data {data_}, _size {size_}, _isEOF(isEOF_) {
    }

    char* data() noexcept  { return _data; }
    size_t size() noexcept { return _size; }
    bool isEOF() noexcept {return _isEOF; }

    explicit operator bool() const noexcept {
      return _size > 0;
    }

    char operator[](int index_) {
      return _data[index_];
    }

    private:

    char* _data;
    size_t _size;
    bool _isEOF;
  };

}}}
