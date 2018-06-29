///////////////////////////////////////////////////////////////////////////////
//
// Assembly code to read time stamp counter from cpu
//
// Logic to estimate frequency of cpu time stamp counter
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <cstdint>
#include <time.h>
#include <unistd.h>

#ifndef RDTSC
#define RDTSC() ({                                                \
  uint32_t lo, hi;                                                \
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));            \
  (uint64_t)hi << 32 | lo;                                        \
})
#endif

#ifndef RDPMC
#define RDPMC(pmu) ({                                             \
  unsigned hi, lo;                                                \
  __asm__ __volatile__("rdpmc" : "=a"(lo), "=d"(hi) : "c"(pmu));  \
  (unsigned long)hi << 32 | lo;                                   \
})        
#endif

namespace xpedite { namespace util {

  inline uint64_t estimateTscHz() noexcept {
    constexpr uint64_t NANOS {1000000000};
    timespec  begin, end;
    auto beginTsc1 = RDTSC();
    if(clock_gettime(CLOCK_REALTIME, &begin)) {
      return {};
    }
    auto endTsc1 = RDTSC();

    usleep(10000);

    auto beginTsc2 = RDTSC();
    if(clock_gettime(CLOCK_REALTIME, &end)) {
      return {};
    }
    auto endTsc2 = RDTSC();

    auto elapsedNsec = (end.tv_sec - begin.tv_sec) * NANOS + (end.tv_nsec - begin.tv_nsec);
    auto elapsedTsc = (beginTsc2 + endTsc2) / 2 - (beginTsc1 + endTsc1) / 2;
    return elapsedTsc * NANOS / elapsedNsec;
  }

}}
