///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite probe definitions
//
// Xpedite support 4 type of macros for instrumenting applications.
//  
//  1. XPEDITE_PROBE - A Named probe to capture timing and pmc data
//
//  2. XPEDITE_DATA_PROBE - A Named probe similar to XPEDITE_PROBE, but also
//     carries 128 bit probe data in addition to timing and pmc counters.
//
//  3. XPEDITE_PROBE_SCOPE - A pair of named probes. The first probe is inserted at
//     the point of instrumentation, while the second one at the end of block scope(RAII).
//
//  4. XPEDITE_DATA_PROBE_SCOPE - A pair of name data probes, with data logged both
//     by both the probes
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

/*
 * A util macro used for building RAII for probes in thread transaction boundaries
 * Declares a struct, that seeds the surrounding scope with 
 * 1. entry probe in the constructor
 * 2. an exit probe in the destructor
 * An automatic variable is defined in place to acheive this
 * The struct preserves a reference (NOT COPY) of the probe data.
 * This is done intentionally to get the side effects (made to probedata)
 * between begin and end probe to be captured by the end probe
 * This means, the reference to probe data must be valid till the end probe is hit
 */

#ifndef XPEDITE_DISABLE
#include <xpedite/probes/ProbeCtl.H>
#include <xpedite/framework/ProbeData.H>

#define CONCAT3_INDIRECT(A, B, C) A##B##C
#define CONCAT3(A, B, C) CONCAT3_INDIRECT(A, B, C)

#define CONCAT2_INDIRECT(A, B) A##B
#define CONCAT2(A, B) CONCAT2_INDIRECT(A, B)

#define XPEDITE_DATA_PROBE_GUARD(STURCT_NAME, PROBE_NAME, DATA)                             \
  struct STURCT_NAME                                                                        \
  {                                                                                         \
    xpedite::framework::ProbeData _cdata;                                                   \
    xpedite::framework::ProbeData& _data;                                                   \
                                                                                            \
    STURCT_NAME(xpedite::framework::ProbeData& data_)                                       \
      : _data (data_) {                                                                     \
      XPEDITE_DATA_PROBE(CONCAT2(PROBE_NAME, Begin), _data);                                \
    }                                                                                       \
    STURCT_NAME(xpedite::framework::ProbeData&& data_)                                      \
      : _cdata{data_}, _data (_cdata) {                                                     \
      XPEDITE_DATA_PROBE(CONCAT2(PROBE_NAME, Begin), _data);                                \
    }                                                                                       \
    ~STURCT_NAME(){                                                                         \
      XPEDITE_DATA_PROBE(CONCAT2(PROBE_NAME, End), _data);                                  \
    }                                                                                       \
  } CONCAT3(STURCT_NAME,_,INSTANCE) {DATA}

#define XPEDITE_PROBE_GUARD(BEGIN_PROBE, END_PROBE, PROBE_NAME, STURCT_NAME)                \
  struct STURCT_NAME                                                                        \
  {                                                                                         \
    STURCT_NAME() {                                                                         \
      BEGIN_PROBE(CONCAT2(PROBE_NAME, Begin));                                              \
    }                                                                                       \
    ~STURCT_NAME() {                                                                        \
      END_PROBE(CONCAT2(PROBE_NAME, End));                                                  \
    }                                                                                       \
  } CONCAT3(STURCT_NAME,_,INSTANCE)

// Create a probe passing probe name as a token
#define XPEDITE_PROBE(NAME) XPEDITE_FLAGGED_PROBE(NAME, 0)

// Create a probe passing probe name and data to be tied to a sample of the probe as arguments
// The data can be built-in integral type or an instance of ProbeData
#define XPEDITE_DATA_PROBE(NAME, ...) XPEDITE_FLAGGED_DATA_PROBE(NAME, \
    static_cast<__uint128_t>(xpedite::framework::ProbeData(__VA_ARGS__)), 0)

#define XPEDITE_PROBE_SCOPE(NAME) XPEDITE_PROBE_GUARD(XPEDITE_PROBE, XPEDITE_PROBE, NAME, \
    CONCAT3(XpediteGuard, NAME, __LINE__))

// Creates a scope with two probes marking begin and end of the enclosing scope
// The probe name and data are passed as arguments
// The data can be built-in integral type or an instance of ProbeData
#define XPEDITE_DATA_PROBE_SCOPE(NAME, ...) XPEDITE_DATA_PROBE_GUARD(CONCAT3(XpediteGuard, NAME, __LINE__), NAME, \
    xpedite::framework::probeData(__VA_ARGS__))

#define XPEDITE_TXN_BEGIN(NAME) XPEDITE_FLAGGED_PROBE(NAME, xpedite::probes::CallSiteAttr::CAN_BEGIN_TXN)

#define XPEDITE_TXN_SUSPEND(NAME) XPEDITE_FLAGGED_IDENTITY_PROBE(NAME, xpedite::probes::CallSiteAttr::CAN_SUSPEND_TXN)

#define XPEDITE_TXN_RESUME(NAME, TXN_ID) XPEDITE_FLAGGED_DATA_PROBE(NAME, static_cast<__uint128_t>(TXN_ID), \
    xpedite::probes::CallSiteAttr::CAN_RESUME_TXN)

#define XPEDITE_TXN_END(NAME) XPEDITE_FLAGGED_PROBE(NAME, xpedite::probes::CallSiteAttr::CAN_END_TXN)

#define XPEDITE_TXN_SCOPE(NAME) XPEDITE_PROBE_GUARD(XPEDITE_TXN_BEGIN, XPEDITE_TXN_END, NAME, CONCAT3(XpediteGuard, NAME, __LINE__))
#else

#define XPEDITE_PROBE(NAME)
#define XPEDITE_DATA_PROBE(NAME, ...)
#define XPEDITE_PROBE_SCOPE(NAME)
#define XPEDITE_DATA_PROBE_SCOPE(NAME, ...)
#define XPEDITE_TXN_BEGIN(NAME)
#define XPEDITE_TXN_SUSPEND(NAME) {}
#define XPEDITE_TXN_RESUME(NAME, TXN_ID)
#define XPEDITE_TXN_END(NAME)
#define XPEDITE_TXN_SCOPE(NAME)

#endif

namespace xpedite { namespace framework {
  using TxnId = __uint128_t;
}}
