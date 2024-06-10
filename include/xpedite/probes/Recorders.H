////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite Probes - Probes with near zero overhead, that can be activated at runtime
//
// This file contains the declarations for diferent recorders.
//
// ExpandAndRecord - expand the samples buffer and record tsc
// recordAndLog    - record tsc and log probe details
// record          - record tsc
// recordPmc       - record tsc, fixed and general performance counters
// recordPerfEvents  - record tsc, pmu events using linux perf events api
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/platform/Builtins.H>
#include <array>
#include <cstdint>

extern "C" {

  void XPEDITE_CALLBACK xpediteExpandAndRecord(const void*, uint64_t);
  void XPEDITE_CALLBACK xpediteRecordAndLog(const void*, uint64_t);
  void XPEDITE_CALLBACK xpediteRecord(const void*, uint64_t);
  void XPEDITE_CALLBACK xpediteRecordPmc(const void*, uint64_t);
  void XPEDITE_CALLBACK xpediteRecordPerfEvents(const void*, uint64_t);

  void XPEDITE_CALLBACK xpediteExpandAndRecordWithData(const void*, uint64_t, __uint128_t);
  void XPEDITE_CALLBACK xpediteRecordWithDataAndLog(const void*, uint64_t, __uint128_t);
  void XPEDITE_CALLBACK xpediteRecordWithData(const void*, uint64_t, __uint128_t);
  void XPEDITE_CALLBACK xpediteRecordPmcWithData(const void*, uint64_t, __uint128_t);
  void XPEDITE_CALLBACK xpediteRecordPerfEventsWithData(const void*, uint64_t, __uint128_t);
}
