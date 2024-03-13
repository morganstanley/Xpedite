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
#include <cstdint>

namespace xpedite { namespace framework {

  class StorageMgr
  {
    const uint64_t _capacity;
    uint64_t _size;

    public:

    static std::string buildSamplesFilePrefix();

    static std::string buildSamplesFileTemplate();

    explicit StorageMgr(uint64_t capacity_)
      : _capacity {capacity_}, _size {} {
      reset();
    }

    void reset();

    uint64_t capacity() const noexcept {
      return _capacity;
    }

    uint64_t consumption() const noexcept {
      return _size;
    }

    bool consume(uint64_t size_) noexcept {
      if(!_capacity) {
        return true;
      }
      if(size_ <= _capacity && _size <= _capacity - size_) {
        _size += size_;
        return true;
      }
      return {};
    }

    void release(uint64_t size_) noexcept {
      _size -= std::min(_size, size_);
    }
  };

}}
