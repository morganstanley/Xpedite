///////////////////////////////////////////////////////////////////////////////
//
// Logic to encode pmu events attributes using linux perf events api
//
// PerfEventSet - A set of perf events, programmed and collected as a group
//
// The attributes set supports customizing the following properties
//  1. Two types of the perf event (PERF_TYPE_HARDWARE and PERF_TYPE_RAW)
//  2. Event select code for a chosen hardware performance counter
//  3. Flags to exclude collection in user/kernel space
//
//  The attributes disable the group leader (first element in the set) by default.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/pmu/EventSet.h>
#include <linux/perf_event.h>
#include <array>
#include <stdexcept>

namespace xpedite { namespace perf {

  struct PerfEventAttrSet {
    std::array<perf_event_attr, XPEDITE_PMC_CTRL_CORE_EVENT_MAX> _values;
    int _size; 
    uint64_t _generation;

    PerfEventAttrSet()
      : _values {}, _size {}, _generation {} {
    }

    void add(perf_event_attr attr_) {
      if(static_cast<unsigned>(_size) >= _values.size()) {
        throw std::runtime_error {"Invariant violation - perf event set exceeded max supported events"};
      }
      attr_.size = sizeof(attr_);
      _values[_size++] = attr_;
    }

    void addPMUEvent(uint32_t eventType_, uint16_t eventSelect_, bool excludeUser_, bool excludeKernel_) {
      perf_event_attr attr {};
      attr.type = eventType_;
      attr.config = eventSelect_;
      attr.exclude_user = excludeUser_;
      attr.exclude_kernel = excludeKernel_;
      attr.disabled = (_size == 0);
      add(attr);
    }

    explicit operator bool() const {
      return _size;
    }

    int size() const noexcept {
      return _size;
    }

    std::string toString() const;
  };

  std::string toString(perf_event_attr attr_);

  PerfEventAttrSet buildPerfEventAttrs(const EventSet& eventSet_) noexcept;

}}
