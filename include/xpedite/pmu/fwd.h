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

#define XPEDITE_LOG(FORMAT_STR, ...) fprintf(stderr, FORMAT_STR, __VA_ARGS__)
