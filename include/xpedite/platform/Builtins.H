///////////////////////////////////////////////////////////////////////////////
//
// Macros to simplify usage of builtins and compiler intrinsics
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef XPEDITE_INLINE
#define XPEDITE_INLINE inline __attribute__((always_inline))
#endif

#ifndef XPEDITE_NOINLINE
#define XPEDITE_NOINLINE __attribute__ ((noinline))
#endif

#ifndef XPEDITE_CALLBACK
#define XPEDITE_CALLBACK __attribute__((__used__, noinline))
#endif

#ifndef XPEDITE_LIKELY
#define XPEDITE_LIKELY(x) __builtin_expect(!!(x), 1)
#endif

#ifndef XPEDITE_UNLIKELY
#define XPEDITE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif 

#ifndef XPEDITE_CACHELINE_SIZE
#define XPEDITE_CACHELINE_SIZE 64
#endif

namespace xpedite { namespace common {

  inline __attribute__((always_inline)) void compilerBarrier() noexcept {
    asm volatile("": : :"memory");
  }

  inline __attribute__((always_inline)) void cpuRelax() noexcept
  {
    asm volatile("rep; nop" ::: "memory");
  }

}}
