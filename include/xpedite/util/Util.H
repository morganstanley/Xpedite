///////////////////////////////////////////////////////////////////////////////
//
// A collection of utility methods to
//
//  1. Pin threads to a cpu
//  2. Capture and build stack trace of the calling thread
//  3. Functionality to convert hex ascii byte pairs to 8 bit numbers
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/log/Log.H>
#include <unistd.h>
#include <sys/syscall.h>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <sstream>
#include <string>
#include <sstream>
#include <sys/prctl.h>
#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <pthread.h>
#include <execinfo.h>
#include <signal.h>
#include <tuple>

namespace xpedite { namespace util {

  std::vector<std::string> listFiles(const char* path_);

  void pinThread(std::thread::native_handle_type handle_, unsigned core_);

  inline pid_t gettid() {
    return syscall(__NR_gettid);
  }

  inline void pinThisThread(unsigned cpu_) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_, &cpuset);
    if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)) {
      throw std::runtime_error {"xpedite - failed to pin thread"};
    }
  }

  inline std::string buildStackTrace(void** btList_, size_t size_) {
    // To capture trace, the statically lined symbols must be exported to dynsym
    // use -rdynamic flag to export all symbols to dynsym section
    auto btStrList = backtrace_symbols(btList_, size_);
    if(btStrList == nullptr) {
      return "failed to read backtrace.";
    }

    std::ostringstream stream;
    for(decltype(size_) i=0; i<size_; ++i) {
      stream << btStrList[i] << std::endl;
    }
    free(btStrList);
    return stream.str();
  }

  inline std::string buildStackTrace() {
    void* btList[128];
    auto size = backtrace(btList, sizeof(btList)/sizeof(btList[0]));
    return buildStackTrace(btList, size);
  }

  inline std::string getProcessName() {
    char tname[32];
    memset(tname, 0, sizeof(tname));
    prctl(PR_GET_NAME, &tname);
    return tname;
  }

  inline std::string getExecutablePath() {
    char buf[4096]; 
    auto len = readlink("/proc/self/exe", buf, sizeof(buf));
    if(len == -1) {
      XpediteLogCritical << "xpedite - error getting binary path" << XpediteLogEnd;
      return "unknown";
    }  
    return {buf, static_cast<size_t>(len)};
  }

  inline int openSamplesFile(const std::string& fname_) {
    auto fd = open(fname_.c_str(), O_WRONLY |O_APPEND |O_TRUNC |O_CREAT, 0644);
    if(fd < 0) {
      std::cerr << "xpedite - error opening samples file '" << fname_ << "' error(" << errno << ") - " << strerror(errno) << std::endl;
    }
    return fd;
  }

  inline void faultHandler(int no_) {
    std::cerr << "--------------------------------------------------------------" << std::endl;
    std::cerr << "terminated by signal (" << no_ << ")" << std::endl;
    std::cerr << buildStackTrace() << std::endl;
    std::cerr << "--------------------------------------------------------------" << std::endl;
    std::exit(no_);
  }

  inline void installFaultHandler() {
    struct sigaction action {};
    action.sa_handler = faultHandler;
    if(sigaction(SIGSEGV, &action, nullptr)) {
      throw std::runtime_error {"failed to register fault handler"};
    }
  }

  inline std::tuple<uint8_t, bool> atoiHex(const char* buffer_) {
    extern uint8_t atoiTable[1 << 8];
    uint8_t msb {atoiTable[static_cast<uint8_t>(buffer_[0])]};
    uint8_t lsb {atoiTable[static_cast<uint8_t>(buffer_[1])]};
    if(msb > 0x0F || lsb > 0x0F) {
      return {};
    }
    return std::make_tuple((msb << 4) + lsb, true);
  }

}}
