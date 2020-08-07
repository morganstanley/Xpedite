///////////////////////////////////////////////////////////////////////////////////////////////
//
// Forward declartion for header inclusion and logging logic
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  xpediteCanLog(void);
extern void xpediteSupressLog(void);
extern void xpediteUnsupressLog(void);

#ifdef __cplusplus
}
#endif

#define XPEDITE_LOG(FORMAT_STR, ...) for(int cl=xpediteCanLog(); cl; cl=0) fprintf(stderr, FORMAT_STR, __VA_ARGS__)
