///////////////////////////////////////////////////////////////////////////////
//
// A stand alone program, demonstrating instrumentation and profiling with Xpedite
// The program can be used to profile latency of random memory acccess vs 
// accessing contiguous memory regions
//
// The file includes logic to
//  1. Generate random numbers
//  2. Initialize Xpedite
//  3. Logic to access memory locations (random or sequential)
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>
#include <xpedite/util/Util.H>
#include <random>
#include <vector>
#include <algorithm>
#include <sys/mman.h>
#include <iostream>

namespace xpedite { namespace demo {

  constexpr uint64_t bufSize = 1 * 1024 * 1024 * 1024;
  constexpr uint64_t indexSize = 2048;
  using Indices = std::array<uint64_t, indexSize>;

  struct RandomNumberGenerator
  {
    std::random_device _randomDevice;
    std::mt19937 _generator;
    std::uniform_int_distribution<> _distribution;
    RandomNumberGenerator(int begin_, int end_) noexcept : _randomDevice {}, _generator{_randomDevice()}, _distribution {begin_, end_} { }
    unsigned next() noexcept { return _distribution(_generator); }
  };

  inline void initialize(size_t core_ = 0) {
    if(mlockall(MCL_CURRENT | MCL_FUTURE)) {
      std::cerr << "failed to mlock all pages" << std::endl;
    }
    if(!xpedite::framework::initialize("xpedite-appinfo.txt", true)) { 
      throw std::runtime_error {"failed to init xpedite"}; 
    }
    framework::pinThread(core_);
  }

  inline void doWork(Indices& indices_, int64_t txnCount_) noexcept {
    std::vector<char> buffer (bufSize, 17);
    for(int i=0; i<txnCount_; ++i) {
      XPEDITE_TXN_SCOPE(Work);
      for(uint64_t j=0; j<indexSize; ++j) {
        auto index = (i*4096 + indices_[j]) % bufSize;
        buffer[index] += j;
      }
    }
  }

  inline int runDemo(int64_t txnCount_, bool randomize_, unsigned cpu_) {
    xpedite::framework::initializeThread();
    util::pinThisThread(cpu_);
    Indices indices;
    if(randomize_) {
      RandomNumberGenerator rng {0, bufSize};
      for(uint64_t i=0; i<indexSize; ++i) { indices[i] = rng.next(); }
    }
    else {
      std::iota(indices.begin(), indices.end(), 0);
    }
    doWork(indices, txnCount_);
    return {};
  }
}}
