///////////////////////////////////////////////////////////////////////////////////////////////
//
// Type definitions to map to layout of event select registers
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "fwd.h"

typedef struct __attribute__ ((__packed__))
{
  union {
    uint32_t    _value;
    struct {
      unsigned char _eventSelect  : 8;
      unsigned char _unitMask     : 8;
      unsigned char _user         : 1;
      unsigned char _kernel       : 1;
      unsigned char _edgeDetect   : 1;
      unsigned char _pinControl   : 1;
      unsigned char _interruptEn  : 1;
      unsigned char _anyThread    : 1;
      unsigned char _enable       : 1;
      unsigned char _invertCMask  : 1;
      unsigned char _counterMask  : 8;
    } _f;
  };
} PerfEvtSelReg;

typedef struct __attribute__ ((__packed__))
{
  union {
    uint32_t    _value;
    struct {
      unsigned char _enable0       : 2;
      unsigned char _anyThread0    : 1;
      unsigned char _interruptEn0  : 1;
      unsigned char _enable1       : 2;
      unsigned char _anyThread1    : 1;
      unsigned char _interruptEn1  : 1;
      unsigned char _enable2       : 2;
      unsigned char _anyThread2    : 1;
      unsigned char _interruptEn2  : 1;
      uint32_t      _reservedBits  : 20;
    } _f;
  };
} FixedEvtSelReg;

