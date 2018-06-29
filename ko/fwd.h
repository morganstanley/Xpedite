///////////////////////////////////////////////////////////////////////////////////////////////
//
// Constant definitions for, the number of programmable pmu registers.
//
//   1. General purpose - 8
//   2. Fixed           - 3
//   3. Offcore         - 2 MSRs
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef XPEDITE_PMU_FWD_H
#define XPEDITE_PMU_FWD_H

#define XPEDITE_PMC_CTRL_GP_EVENT_MAX 8

#define XPEDITE_PMC_CTRL_FIXED_EVENT_MAX 3

#define XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX 2

#endif
