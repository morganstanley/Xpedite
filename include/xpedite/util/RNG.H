///////////////////////////////////////////////////////////////////////////////
//
// A utility to generate random numbers
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <random>

namespace xpedite { namespace util {

  struct RandomNumberGenerator
  {
    std::random_device _randomDevice;
    std::mt19937 _generator;
    std::uniform_int_distribution<> _distribution;

    RandomNumberGenerator(int begin_, int end_) noexcept
      : _randomDevice {}, _generator{_randomDevice()}, _distribution {begin_, end_} {
    }

    unsigned next() noexcept {
      return _distribution(_generator);
    }
  };

}}
