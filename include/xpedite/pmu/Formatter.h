///////////////////////////////////////////////////////////////////////////////////////////////
//
// Formatters to convert pmu request and events to strings
// 
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "fwd.h"
#include <xpedite/pmu/EventSet.h>

#ifdef __cplusplus
extern "C" {
#endif

  void logEventSet(const PMUCtlRequest* request_, const EventSet* eventSet_);

  void pmuRequestToString(const PMUCtlRequest* request_, char* buffer_, int size_);

#ifdef __cplusplus
}
#endif
