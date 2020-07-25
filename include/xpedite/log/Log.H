///////////////////////////////////////////////////////////////////////////////
//
// Macros to provide abstractions for logging
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once 
#include <iostream>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <chrono>
#include <ostream>

namespace xpedite { namespace probes {
  class Probe;
  class ProbeList;
}}

namespace xpedite { namespace log {

  constexpr bool isDebug() {
#ifndef NDEBUG
    return true;
#else
    return false;
#endif
  }

  extern uint64_t logCounter;

  inline char* _logTime() {
    static thread_local char buffer[80];
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);

    strftime(buffer, 80, "%c", timeinfo);
    return buffer;
  }

  void logProbe(const probes::Probe& probe_, const char* action_ = {});

  void logProbe(std::ostream& logfile_, const probes::Probe& probe_, const char* action_ = {});

  void logProbes(const probes::ProbeList& probeList_);

  void logProbes(std::ostream& logfile_, const probes::ProbeList& probeList_);
}}

#define XPEDITE_LOG_PREFIX1 __sync_add_and_fetch(&xpedite::log::logCounter, 1)
#define XPEDITE_LOG_PREFIX2 xpedite::log::_logTime()
#define XPEDITE_LOG_POSTFIX ""

// redirect the logging macros which allows to switch the implementation
// when it's running with unit test, below macro should direct the logging to std::cout or std::cerr instead
#define XpediteLogCritical std::cout << XPEDITE_LOG_PREFIX1 << " critical " << XPEDITE_LOG_PREFIX2 << " "
#define XpediteLogError    std::cout << XPEDITE_LOG_PREFIX1 << " error "    << XPEDITE_LOG_PREFIX2 << " "
#define XpediteLogWarning  std::cout << XPEDITE_LOG_PREFIX1 << " warning "  << XPEDITE_LOG_PREFIX2 << " "
#define XpediteLogInfo     std::cout << XPEDITE_LOG_PREFIX1 << " info "     << XPEDITE_LOG_PREFIX2 << " "
#define XpediteLogDebug    for(bool cl=xpedite::log::isDebug(); cl; cl=false) std::cout << XPEDITE_LOG_PREFIX1 << " debug " << XPEDITE_LOG_PREFIX2 << " "
#define XpediteLogEnd      XPEDITE_LOG_POSTFIX << std::endl //" (" << __BASE_FILE__  << ":" << __LINE__ << ")" << std::endl

