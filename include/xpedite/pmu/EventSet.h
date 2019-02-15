///////////////////////////////////////////////////////////////////////////////////////////////
//
// Provides type definitions for recieving pmu requests from user space.
// 
// PMUFixedEvent - defines programmable attributes of Fixed pmc events
//
// PMUGpEvent - defines programmable attributes of general purpose pmc events
//
// PMUCtlRequest - Collection of pmc events to be programmed
//
// EventSelect - A machine friendly representation of programmable pmc events
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "fwd.h"

#define XPEDITE_PMC_CTRL_GP_EVENT_MAX 8

#define XPEDITE_PMC_CTRL_FIXED_EVENT_MAX 3

#define XPEDITE_PMC_CTRL_CORE_EVENT_MAX 11

#define XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX 2

typedef struct __attribute__ ((__packed__))
{
  unsigned char _ctrIndex;
  unsigned char _user;
  unsigned char _kernel;
} PMUFixedEvent;

typedef struct __attribute__ ((__packed__))
{
  unsigned char _eventSelect;
  unsigned char _unitMask;
  unsigned char _user;
  unsigned char _kernel;
  unsigned char _invertCMask;
  unsigned char _counterMask;
  unsigned char _edgeDetect;
  unsigned char _anyThread;
} PMUGpEvent;

typedef uint64_t PMUOffcoreEvent;
 
typedef struct __attribute__ ((__packed__))
{
  unsigned char _cpu;
  unsigned char _fixedEvtCount;
  unsigned char _gpEvtCount;
  unsigned char _offcoreEvtCount;
  PMUFixedEvent _fixedEvents[XPEDITE_PMC_CTRL_FIXED_EVENT_MAX];
  PMUGpEvent _gpEvents[XPEDITE_PMC_CTRL_GP_EVENT_MAX];
  PMUOffcoreEvent _offcoreEvents[XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX];
} PMUCtlRequest;

typedef struct
{
  unsigned char _fixedEvtGlobalCtl;
  unsigned char _gpEvtCount;
  unsigned char _offcoreEvtCount;
  uint32_t _fixedEvtSel;
  uint32_t _gpEvtSel[XPEDITE_PMC_CTRL_GP_EVENT_MAX];
  uint64_t _offcoreEvtSel[XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX];
  ssize_t _err;
} EventSet;

#ifdef __cplusplus
extern "C" {
#endif
  int buildEventSet(const PMUCtlRequest* r_, EventSet* eventSet_);

  int eventCount(const PMUCtlRequest* request_);

  unsigned char maskEnabledInUserSpace(unsigned char mask_);

  unsigned char maskEnabledInKernel(unsigned char mask_);

#ifdef __cplusplus
}
#endif
