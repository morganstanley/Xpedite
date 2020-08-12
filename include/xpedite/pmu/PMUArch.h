///////////////////////////////////////////////////////////////////////////////////////////////
//
// Provides enumerations for MSR addresses of programmable pmu registers
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "fwd.h"
#include "EventSet.h"

typedef enum 
{
  PMU_MSR_IA32_PEBS_ENABLE        = 0x3F1,
  PMU_MSR_IA32_PERF_GLOBAL_CTRL   = 0x38F,
  PMU_MSR_IA32_FIXED_CTR_CTRL     = 0x38D,

  PMU_MSR_IA32_FIXED_CTR0         = 0x309,
  PMU_MSR_IA32_FIXED_CTR1         = 0x30A,
  PMU_MSR_IA32_FIXED_CTR2         = 0x30B,

  PMU_MSR_PerfEvtSel0             = 0x186,
  PMU_MSR_PerfEvtSel1             = 0x187,
  PMU_MSR_PerfEvtSel2             = 0x188,
  PMU_MSR_PerfEvtSel3             = 0x189,
  PMU_MSR_PerfEvtSel4             = 0x18A,
  PMU_MSR_PerfEvtSel5             = 0x18B,
  PMU_MSR_PerfEvtSel6             = 0x18C,
  PMU_MSR_PerfEvtSel7             = 0x18D,

  PMU_MSR_IA32_PMC0                = 0xC1,
  PMU_MSR_IA32_PMC1                = 0xC2,
  PMU_MSR_IA32_PMC2                = 0xC3,
  PMU_MSR_IA32_PMC3                = 0xC4,
  PMU_MSR_IA32_PMC4                = 0xC5,
  PMU_MSR_IA32_PMC5                = 0xC6,
  PMU_MSR_IA32_PMC6                = 0xC7,
  PMU_MSR_IA32_PMC7                = 0xC8,

  PMU_MSR_OFFCORE_RSP_0            = 0x1A6,
  PMU_MSR_OFFCORE_RSP_1            = 0x1A7,
} PMUMsr;

extern ssize_t pmuEnableEventSet(EventSet* eventSet);

extern ssize_t pmuClearEventSet(unsigned char gpEvtCount_);
