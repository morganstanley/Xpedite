///////////////////////////////////////////////////////////////////////////////////////////////
//
// Provides type definitions for recieving pmu requests from user space.
// 
// PMUFixedEvent - defines programmable attributes of Fixed pmc events
//
// PMUGpEvent - defines programmable attributes of general purpose pmc events
//
// PMUCtrlRequest - Collection of pmc events to be programmed
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef XPEDITE_PMU_CTRL_H
#define XPEDITE_PMU_CTRL_H
#include "fwd.h"

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
} PMUCtrlRequest;

#endif

