//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ProbeData is a plain old datatype, that provides a structured interface for
// get/set operations on 16 byte buffer.
// 
// This type is used to attach data to a xpedite probes, typically at thread transactional boundaries
// Type punning safe implementation to manipulate 16 bytes as bytes, words, double words, quad words
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <array>
#include <cassert>
#include <initializer_list>

namespace xpedite { namespace framework {

  class ProbeData
  {
    static constexpr int CAPACITY = sizeof(__uint128_t);

    public:

    ProbeData() = default;

    template<typename T>
    ProbeData(std::initializer_list<T> value_) {
      set(value_);
    }

    template<typename T> T* data() noexcept;

    template<typename T> const T* data() const noexcept {
      return const_cast<ProbeData*>(this)->data<T>();
    }

    template<typename T, std::uint8_t Index, typename std::enable_if<(sizeof(T)*(Index+1)<= CAPACITY), int>::type = 0>
    T get() const noexcept {
      return data<T>()[Index];
    }

    template<typename T, std::uint8_t Index, typename std::enable_if<(sizeof(T)*(Index+1)>CAPACITY), int>::type = 0>
    T get() const noexcept {
      static_assert(sizeof(T)*(Index+1)<=CAPACITY, "Detected out of bounds access");
      return {};
    }

    template<typename T, std::uint8_t Index, typename std::enable_if<sizeof(T) * (Index+1) <= CAPACITY, int>::type = 0>
    void set(T value_) noexcept {
      data<T>()[Index] = value_;
    }

    template<typename T, std::uint8_t Index, typename std::enable_if<(sizeof(T)*(Index+1)>CAPACITY), int>::type = 0>
    void set(T) noexcept {
      static_assert(sizeof(T)*(Index+1)<=CAPACITY, "Detected out of bounds access");
    }

    template<typename T, std::size_t N, typename std::enable_if<std::is_pod<T>::value && sizeof(T) * N <= CAPACITY, int>::type = 0>
    void set(T (&arr)[N]) { 
      memcpy(_bytes, arr, sizeof(T) * N);
    }

    template<typename T, std::size_t N, typename std::enable_if<std::is_pod<T>::value && sizeof(T) * N <= CAPACITY, int>::type = 0>
    void set(const std::array<T, N>& blob_) { 
      memcpy(_bytes, blob_.data(), sizeof(T) * blob_.size());
    }

    template<typename T>
    void set(std::initializer_list<T> blob_) { 
      assert(sizeof(T) * blob_.size() <= CAPACITY);
      std::copy(blob_.begin(), blob_.end(), data<T>());
    }

    explicit operator __uint128_t() const {
      return _doubleQuad;
    }

    private:

    union
    {
      std::uint8_t _bytes[CAPACITY/sizeof(std::uint8_t)];
      std::uint16_t _words[CAPACITY/sizeof(std::uint16_t)];
      std::uint32_t _doubleWords[CAPACITY/sizeof(std::uint32_t)];
      std::uint64_t _quadWords[CAPACITY/sizeof(std::uint64_t)];
      __uint128_t _doubleQuad;
    };
  };

  template<> inline std::uint8_t*  ProbeData::data<std::uint8_t>()  noexcept { return _bytes;       }
  template<> inline std::uint16_t* ProbeData::data<std::uint16_t>() noexcept { return _words;       }
  template<> inline std::uint32_t* ProbeData::data<std::uint32_t>() noexcept { return _doubleWords; }
  template<> inline std::uint64_t* ProbeData::data<std::uint64_t>() noexcept { return _quadWords;   }
}}
