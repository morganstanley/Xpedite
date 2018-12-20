///////////////////////////////////////////////////////////////////////////////
//
// FixedPmcSet - Object to track state of fixed pmc registers 
//
// Supports profiling of three architectual fixed counter registers
//   1. Number of instructions retired
//   2. Count of cycles, when core is not halted
//   2. Count of reference cycles, when core is not halted
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <sstream>
#include <string>
#include <stdexcept>
#include <array>

namespace xpedite { namespace pmu {

  class FixedPmcSet
  {
    uint8_t _counterSet;
    uint8_t _size;

    public:

    enum {
      INST_RETIRED_ANY,
      CPU_CLK_UNHALTED_CORE,
      CPU_CLK_UNHALTED_REF,
      MAX_COUNTER_COUNT
    };

    FixedPmcSet()
      : _counterSet {}, _size {} {
    }

    template<uint8_t CounterIndex>
    void enable() noexcept {
      static_assert(CounterIndex < MAX_COUNTER_COUNT, "Invalid fixed pmu counter index");
      constexpr uint8_t mask {1 << CounterIndex};
      _counterSet |= mask; 
      _size = __builtin_popcount(_counterSet);
    }

    void enable(uint8_t index_) {
      switch(index_) {
        case INST_RETIRED_ANY:
          enable<INST_RETIRED_ANY>();
          break;
        case CPU_CLK_UNHALTED_CORE:
          enable<CPU_CLK_UNHALTED_CORE>();
          break;
        case CPU_CLK_UNHALTED_REF:
          enable<CPU_CLK_UNHALTED_REF>();
          break;
        default:
          throw std::runtime_error {"xpedite failed to enable fixed pmu counter - invalid fixed pmu counter index."};
      }
    }

    template<uint8_t CounterIndex>
    void disable() noexcept {
      static_assert(CounterIndex < MAX_COUNTER_COUNT, "Invalid fixed pmu counter index");
      constexpr uint8_t mask {~(1 << CounterIndex)};
      _counterSet &= mask; 
      _size = __builtin_popcount(_counterSet);
    }

    template<uint8_t CounterIndex>
    bool isEnabled() const noexcept {
      static_assert(CounterIndex < MAX_COUNTER_COUNT, "Invalid fixed pmu counter index");
      constexpr uint8_t mask {1 << CounterIndex};
      return _counterSet & mask; 
    }

    void reset() noexcept {
      _counterSet = 0;
      _size = 0;
    }

    uint8_t size() const noexcept {
      return _size;
    }
    
    std::string toString() const {
      static std::array<const char*, MAX_COUNTER_COUNT +1> counterNames {"INST_RETIRED_ANY", "CPU_CLK_UNHALTED_CORE", "CPU_CLK_UNHALTED_REF", "UNKNOWN"};
      std::ostringstream stream;
      stream << "Enabled fixed PMU Counters - | ";
      if(!_counterSet) {
        stream << "NONE |";
      }
      else {
        for(uint8_t i=0; i<MAX_COUNTER_COUNT; ++i) {
          uint8_t mask {static_cast<uint8_t>(1 << i)};
          if(_counterSet & mask) {
            stream << counterNames[i] << " | ";
          }
        }
      }
      return stream.str();
    }
  };

}}
