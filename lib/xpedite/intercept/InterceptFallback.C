///////////////////////////////////////////////////////////////////////////////
//
// Provides fallback for memory allocation wrappers, when not in use
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <cassert>
#include <stdexcept>

extern "C"
{
  void* __real__Znwm(size_t) {
    throw std::runtime_error {"intercept failure - failed to forward allocation request to real new"};
  }

  void* __real__Znam(size_t) {
    throw std::runtime_error {"intercept failure - failed to forward allocation request to real new []"};
  }

  void* __real_malloc(size_t) {
    throw std::runtime_error {"intercept failure - failed to forward allocation request to real malloc"};
  }

  void* __real_calloc(size_t, size_t) {
    throw std::runtime_error {"intercept failure - failed to forward allocation request to real calloc"};
  }


  void* __real_realloc(void*, size_t) {
    throw std::runtime_error {"intercept failure - failed to forward allocation request to real realloc"};
  }

  int __real_posix_memalign(void**, size_t, size_t) {
    throw std::runtime_error {"intercept failure - failed to forward allocation request to real posix_memalign"};
  }


  void* __real_aligned_alloc(size_t, size_t) {
    throw std::runtime_error {"intercept failure - failed to forward allocation request to real aligned_alloc"};
  }


  void* __real_valloc(size_t) {
    throw std::runtime_error {"intercept failure - failed to forward allocation request to real valloc"};
  }

  void __real_free(void*) {
    throw std::runtime_error {"intercept failure - failed to forward deallocation request to real free"};
  }

  void* __real_mmap(void*, size_t, int, int, int, off_t) {
    throw std::runtime_error {"intercept failure - failed to forward allocation request to real mmap"};
  }

  int __real_munmap(void*, size_t) {
    throw std::runtime_error {"intercept failure - failed to forward deallocation request to real munmap"};
  }
}
