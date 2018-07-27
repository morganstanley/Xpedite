///////////////////////////////////////////////////////////////////////////////
//
// The file contains 
//  1. Classes and methods to provide custom memory allocators
//  2. Classes to enforce strict alignment of latency critical objects
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/platform/Builtins.H>
#include <stdexcept>
#include <sys/mman.h>
#include <xpedite/util/Util.H>

namespace xpedite { namespace util {

  inline void* xpediteMalloc(size_t size) {
    size = (size / 4096 + 1) * 4096;
    void* ptr {};
    if((ptr = static_cast<void*>(mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))) == MAP_FAILED) {
      return nullptr;
    }
    memset(ptr, 0, size);
    return ptr;
  }

  inline void xpediteFree(void* ptr_, size_t size_) {
    munmap(ptr_, size_);
  }

  template<typename T, typename... Args>
  inline T* xpediteNew(Args&&... args) {
    auto p = xpediteMalloc(sizeof(T));
    if(p) {
      return new(p) T {std::forward<Args>(args)...};
    }
    throw std::bad_alloc {};
  }

  template<typename T>
  inline void xpediteDelete(T* ptr_) {
    if(ptr_) {
      ptr_->~T();
    }
    xpediteFree(ptr_);
  }

  template<typename T>
  struct Allocator
  {
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind {
      using other = Allocator<U>;
    };

    pointer allocate(size_type n_, std::allocator<void>::const_pointer hint_ = 0) {
      const auto size = n_ * sizeof(T);
      if(auto ptr = xpediteMalloc(size)) {
        return reinterpret_cast<pointer>(ptr);
      }
      throw std::bad_alloc {};
    }

    void deallocate(pointer p_, size_type n_) {
      xpediteFree(p_);
    }

    template<typename U, typename... Args>
    void construct(U * p_, Args&&... args_) {
      new (p_) U {std::forward<Args>(args_)...};
    }

    template<typename U>
    void destroy(U * p_) {
      p_->~U();
    }

    size_type max_size() const noexcept {
      return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }

  };

  template<size_t ALIGNMENT = XPEDITE_CACHELINE_SIZE>
  struct alignas(ALIGNMENT) AlignAs
  {
    static void* operator new(size_t size_) {
      void *ptr;
      if(!posix_memalign(&ptr, ALIGNMENT, size_)) {
        return ptr;
      }
      else {
        throw std::bad_alloc();
      }
    }

    static void* operator new[](size_t size_) {
      void *ptr;
      if(!posix_memalign(&ptr, ALIGNMENT, size_)) {
        return ptr;
      }
      else {
        throw std::bad_alloc();
      }
    }

    static void operator delete(void* ptr_) {
      free(ptr_);
    }

    static void operator delete[](void* ptr_) {
      free(ptr_);
    }
  };

  template <std::size_t ALIGNMENT = XPEDITE_CACHELINE_SIZE>
  class alignas(ALIGNMENT) AlignedObject : public AlignAs<ALIGNMENT>
  {
    public:

    AlignedObject() {
      // check alignment
      if(reinterpret_cast<uintptr_t>(this) & (ALIGNMENT - 1)) {
        std::ostringstream stream;
        stream << "Object allocated at unalined address (" << this << "). - expected alignment " << ALIGNMENT << " bytes.";
        throw std::runtime_error {stream.str()};
      }
    }

    // AlignedObject cannot be copied or moved
    AlignedObject(const AlignedObject&) = delete;
    AlignedObject& operator=(const AlignedObject&) = delete;
    AlignedObject(AlignedObject&&) = delete;
    AlignedObject& operator=(AlignedObject&&) = delete;
    ~AlignedObject() = default;
  };
}}
