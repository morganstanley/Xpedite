///////////////////////////////////////////////////////////////////////////////
//
// Provides wrapper impementations for common memory allocation methods
// The wrappers are instrumented with Xpedite probes to 
// intercept and report memory allocations in critical path
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/util/Util.H>
#include <xpedite/platform/Builtins.H>
#include <xpedite/intercept/Report.H>
#include <xpedite/framework/Probes.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <cstddef>

namespace xpedite { namespace intercept {
  void interceptOp(const char* op, void* mem, std::size_t size = -1);
}}

using xpedite::intercept::interceptOp;

extern "C"
{
  void* __real__Znwm(size_t size_);
  void* __wrap__Znwm(size_t size_) {
    // to avoid recursive calls to malloc, if the thread local memory for probe gets lazily initialized
    if(XPEDITE_LIKELY(xpedite::framework::SamplesBuffer::isInitialized())) {
      XPEDITE_PROBE_SCOPE(New);
    }
    auto ptr = __real__Znwm(size_);
    interceptOp("new", ptr, size_);
    return ptr;
  }

  void* __real__Znam(size_t size_);
  void* __wrap__Znam(size_t size_) {
    if(XPEDITE_LIKELY(xpedite::framework::SamplesBuffer::isInitialized())) {
      XPEDITE_PROBE_SCOPE(New);
    }
    auto ptr = __real__Znam(size_);
    interceptOp("new []", ptr, size_);
    return ptr;
  }

  void* __real_malloc(size_t size_);
  void* __wrap_malloc(size_t size_) {
    if(XPEDITE_LIKELY(xpedite::framework::SamplesBuffer::isInitialized())) {
      XPEDITE_PROBE_SCOPE(Malloc);
    }
    auto ptr = __real_malloc(size_);
    interceptOp("malloc", ptr, size_);
    return ptr;
  }

  void* __real_calloc(size_t num_, size_t size_);
  void* __wrap_calloc(size_t num_, size_t size_) {
    if(XPEDITE_LIKELY(xpedite::framework::SamplesBuffer::isInitialized())) {
      XPEDITE_PROBE_SCOPE(Calloc);
    }
    auto ptr = __real_calloc(num_, size_);
    interceptOp("calloc", ptr, size_);
    return ptr;
  }

  void* __real_realloc(void* ptr_, size_t new_size_);
  void* __wrap_realloc(void* ptr_, size_t new_size_) {
    XPEDITE_PROBE_SCOPE(Realloc);
    auto ptr = __real_realloc(ptr_, new_size_);
    interceptOp("realloc", ptr, new_size_);
    return ptr;
  }

  int __real_posix_memalign(void** memptr_, size_t alignment_, size_t size_);
  int __wrap_posix_memalign(void** memptr_, size_t alignment_, size_t size_) {
    if(XPEDITE_LIKELY(xpedite::framework::SamplesBuffer::isInitialized())) {
      XPEDITE_PROBE_SCOPE(PosixMemalign);
    }
    auto rc = __real_posix_memalign(memptr_, alignment_, size_);
    interceptOp("posix_memalign", *memptr_, size_);
    return rc;
  }

  void* __real_aligned_alloc(size_t alignment_, size_t size_);
  void* __wrap_aligned_alloc(size_t alignment_, size_t size_) {
    XPEDITE_PROBE_SCOPE(AlignedAlloc);
    auto ptr = __real_aligned_alloc(alignment_, size_);
    interceptOp("aligned_alloc", ptr, size_);
    return ptr;
  }

  void* __real_valloc(size_t size_);
  void* __wrap_valloc(size_t size_) {
    XPEDITE_PROBE_SCOPE(Valloc);
    auto ptr = __real_valloc(size_);
    interceptOp("valloc", ptr, size_);
    return ptr;
  }

  void __real_free(void* ptr_);
  void __wrap_free(void* ptr_) {
    XPEDITE_PROBE_SCOPE(Free);
    __real_free(ptr_);
    interceptOp("free", ptr_);
  }

  void* __real_mmap(void* addr_, size_t length_, int prot_, int flags_, int fd_, off_t offset_);
  void* __wrap_mmap(void* addr_, size_t length_, int prot_, int flags_, int fd_, off_t offset_) {
    // to avoid recursive calls to malloc, if the thread local memory for probe gets lazily initialized
    if(XPEDITE_LIKELY(xpedite::framework::SamplesBuffer::isInitialized())) {
      XPEDITE_PROBE_SCOPE(Mmap);
    }
    auto ptr = __real_mmap(addr_, length_, prot_, flags_, fd_, offset_);
    interceptOp("mmap", ptr, length_);
    return ptr;
  }

  int __real_munmap(void* addr_, size_t length_);
  int __wrap_munmap(void* addr_, size_t length_) {
    XPEDITE_PROBE_SCOPE(Munmap);
    auto rc = __real_munmap(addr_, length_);
    interceptOp("munmap", addr_, length_);
    return rc;
  }
}
