///////////////////////////////////////////////////////////////////////////////
//
// A utility class to control storage for xpedite samples data
//
// The storage manager keeps track of current memory/file system consumption.
// It also provides methods to build file system paths for different data files
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>

namespace xpedite { namespace framework {

  class StorageMgr
  {
    const int _capacity;
    int _size;

    public:

    static std::string buildSamplesFilePrefix();

    static std::string buildSamplesFileTemplate();

    explicit StorageMgr(int capacity_)
      : _capacity {capacity_}, _size {} {
      reset();
    }

    void reset();

    int capacity() const noexcept {
      return _capacity;
    }

    int consumption() const noexcept {
      return _size;
    }

    bool consume(int size_) noexcept {
      if(_capacity < 0) {
        return true;
      }
      if(size_ <= _capacity && _size <= _capacity - size_) {
        _size += size_;
        return true;
      }
      return {};
    }

    void release(int size_) noexcept {
      _size -= std::min(_size, size_);
    }
  };

}}
