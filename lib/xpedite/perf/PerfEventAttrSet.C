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

#include <xpedite/pmu/EventSelect.h>
#include <xpedite/pmu/FixedPmcSet.H>
#include <xpedite/perf/PerfEventAttrSet.H>
#include <xpedite/log/Log.H>

namespace xpedite { namespace perf {

  PerfEventAttrSet buildPerfEventAttrs(const EventSet& eventSet_) noexcept {
    PerfEventAttrSet perfEventAttrSet {};

    for(int i=0; i < eventSet_._gpEvtCount; ++i) {
      PerfEvtSelReg reg {};
      reg._value = eventSet_._gpEvtSel[i];
      uint16_t eventSelect (reg._f._unitMask << 8 | reg._f._eventSelect);
      perfEventAttrSet.addPMUEvent(PERF_TYPE_RAW, eventSelect, !reg._f._user, !reg._f._kernel);
    }

    FixedEvtSelReg fixedEvtSelReg {};
    fixedEvtSelReg._value = eventSet_._fixedEvtSel;

    using FixedPmcSet = pmu::FixedPmcSet;
    if(eventSet_._fixedEvtGlobalCtl & (0x1 << FixedPmcSet::INST_RETIRED_ANY)) {
      bool excludeUser = !maskEnabledInUserSpace(fixedEvtSelReg._f._enable0);
      bool excludeKernel = !maskEnabledInKernel(fixedEvtSelReg._f._enable0);
      perfEventAttrSet.addPMUEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, excludeUser, excludeKernel);
    }

    if(eventSet_._fixedEvtGlobalCtl & (0x1 << FixedPmcSet::CPU_CLK_UNHALTED_CORE)) {
      bool excludeUser = !maskEnabledInUserSpace(fixedEvtSelReg._f._enable1);
      bool excludeKernel = !maskEnabledInKernel(fixedEvtSelReg._f._enable1);
      perfEventAttrSet.addPMUEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, excludeUser, excludeKernel);
    }

    if(eventSet_._fixedEvtGlobalCtl & (0x1 << FixedPmcSet::CPU_CLK_UNHALTED_REF)) {
      // https://lwn.net/articles/373473
      bool excludeUser = !maskEnabledInUserSpace(fixedEvtSelReg._f._enable2);
      bool excludeKernel = !maskEnabledInKernel(fixedEvtSelReg._f._enable2);
      perfEventAttrSet.addPMUEvent(PERF_TYPE_RAW, 0x13c, excludeUser, excludeKernel);
    }
    return perfEventAttrSet;
  }

  const char* eventTypeToString(uint32_t eventType_) {
    switch(eventType_) {
      case PERF_TYPE_HARDWARE:
        return "Hardware";
      case PERF_TYPE_RAW:
        return "Raw";
      case PERF_TYPE_SOFTWARE:
        return "Software";
      case PERF_TYPE_TRACEPOINT:
        return "Tracepoint";
      case PERF_TYPE_BREAKPOINT:
        return "Breakpoint";
    }
    return "Unknown";
  }

  std::string toString(perf_event_attr attr_) {
    const char* type = eventTypeToString(attr_.type);
    std::ostringstream os;
    os << "Event [type - " << type << " | config - " << std::hex << attr_.config << std::boolalpha
      << " | excludes user - " << attr_.exclude_user << " | excludes kernel - " << attr_.exclude_kernel << "]"
      << " -> disabled - " << attr_.disabled;
    return os.str();
  }

  std::string PerfEventAttrSet::toString() const {
    std::ostringstream os;
    for(int i=0; i < _size; ++i) {
      os << xpedite::perf::toString(_values[i]) << "\n";
    }
    return os.str();
  }

}}
