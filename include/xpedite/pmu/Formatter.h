///////////////////////////////////////////////////////////////////////////////////////////////
//
// Formatters to convert pmu request and events to strings
// 
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "fwd.h"
#include <xpedite/pmu/EventSelect.h>

#ifdef __cplusplus
extern "C" {
#endif

  void logRequest(unsigned ctrIndex_, const PMUGpEvent* e_, uint32_t b_);

  void logOffcoreRequest(unsigned ctrIndex_, uint64_t e_);

  int pmcrqToString(const PMUCtlRequest* request_, char* buffer_, int size_);

#ifdef __cplusplus
}
#endif
