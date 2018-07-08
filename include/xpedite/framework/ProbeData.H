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

  template<uint8_t Size, typename A, typename... Args>
  struct SameSize
  {
    static constexpr bool value {sizeof(A) == Size && SameSize<Size, Args...>::value};
  };

  template<uint8_t Size, typename A>
  struct SameSize<Size, A>
  {
    static constexpr bool value {sizeof(A) == Size};
  };

  class ProbeData
  {
    static constexpr int CAPACITY = sizeof(__uint128_t);

    public:

    ProbeData() = default;

    template<typename T, typename... Args, typename std::enable_if<SameSize<sizeof(uint8_t), T, Args...>::value, int>::type = 0>
    explicit ProbeData(T t_, Args... args_) noexcept
      : _bytes {t_, args_...} {
      static_assert(sizeof...(Args) < sizeof(_bytes)/sizeof(_bytes[0]), "Exceeds capacity of probe data");
    }

    template<typename T, typename... Args, typename std::enable_if<SameSize<sizeof(uint16_t), T, Args...>::value, int>::type = 0>
    explicit ProbeData(T t_, Args... args_) noexcept
      : _words {t_, args_...} {
      static_assert(sizeof...(Args) < sizeof(_words)/sizeof(_words[0]), "Exceeds capacity of probe data");
    }

    template<typename T, typename... Args, typename std::enable_if<SameSize<sizeof(uint32_t), T, Args...>::value, int>::type = 0>
    explicit ProbeData(T t_, Args... args_) noexcept
      : _doubleWords {t_, args_...} {
      static_assert(sizeof...(Args) < sizeof(_doubleWords)/sizeof(_doubleWords[0]), "Exceeds capacity of probe data");
    }

    template<typename T, typename... Args, typename std::enable_if<SameSize<sizeof(uint64_t), T, Args...>::value, int>::type = 0>
    explicit ProbeData(T t_, Args... args_) noexcept
      : _quadWords {t_, args_...} {
      static_assert(sizeof...(Args) < sizeof(_quadWords)/sizeof(_quadWords[0]), "Exceeds capacity of probe data");
    }

    template<typename T, typename... Args, typename std::enable_if<SameSize<sizeof(__uint128_t), T, Args...>::value, int>::type = 0>
    explicit ProbeData(T t_, Args...) noexcept
      : _doubleQuad {t_} {
      static_assert(sizeof...(Args) == 0, "Exceeds capacity of probe data");
    }

    template<typename T> T* data() noexcept;

    template<typename T> const T* data() const noexcept {
      return const_cast<ProbeData*>(this)->data<T>();
    }

    template<typename T, uint8_t Index, typename std::enable_if<(sizeof(T)*(Index+1)<= CAPACITY), int>::type = 0>
    T get() const noexcept {
      return data<T>()[Index];
    }

    template<typename T, uint8_t Index, typename std::enable_if<(sizeof(T)*(Index+1)>CAPACITY), int>::type = 0>
    T get() const noexcept {
      static_assert(sizeof(T)*(Index+1)<=CAPACITY, "Detected out of bounds access");
      return {};
    }

    template<typename T, uint8_t Index, typename std::enable_if<sizeof(T) * (Index+1) <= CAPACITY, int>::type = 0>
    void set(T value_) noexcept {
      data<T>()[Index] = value_;
    }

    template<typename T, uint8_t Index, typename std::enable_if<(sizeof(T)*(Index+1)>CAPACITY), int>::type = 0>
    void set(T) noexcept {
      static_assert(sizeof(T)*(Index+1)<=CAPACITY, "Detected out of bounds access");
    }

    template<typename T, std::size_t N, typename std::enable_if<std::is_pod<T>::value && sizeof(T) * N <= CAPACITY, int>::type = 0>
    void set(T (&arr)[N]) { 
      memcpy(_bytes, arr, sizeof(T) * N);
    }

    explicit operator __uint128_t() const {
      return _doubleQuad;
    }

    private:

    union
    {
      uint8_t _bytes[CAPACITY/sizeof(uint8_t)];
      uint16_t _words[CAPACITY/sizeof(uint16_t)];
      uint32_t _doubleWords[CAPACITY/sizeof(uint32_t)];
      uint64_t _quadWords[CAPACITY/sizeof(uint64_t)];
      __uint128_t _doubleQuad;
    };
  };

  template<> inline uint8_t*  ProbeData::data<uint8_t>()  noexcept { return _bytes;       }
  template<> inline uint16_t* ProbeData::data<uint16_t>() noexcept { return _words;       }
  template<> inline uint32_t* ProbeData::data<uint32_t>() noexcept { return _doubleWords; }
  template<> inline uint64_t* ProbeData::data<uint64_t>() noexcept { return _quadWords;   }

  inline ProbeData& probeData(ProbeData& probeData_) {
    return probeData_;
  }

  template<typename... Args>
  inline ProbeData probeData(Args&&... args_) {
    return ProbeData {std::forward<Args>(args_)...};
  }
}}
