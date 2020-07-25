///////////////////////////////////////////////////////////////////////////////////////////////
//
// Utility class to redirect/capture cout within a scope of execution
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <ostream>
#include <sstream>

extern "C" void xpediteSupressLog();
extern "C" void xpediteUnsupressLog();

struct LogSupressScope
{
  LogSupressScope()
    : _stream {}, _coutBuf {std::cout.rdbuf()} {
    std::cout.rdbuf(_stream.rdbuf());
    xpediteSupressLog();
  }

  LogSupressScope(const LogSupressScope&)            = delete;
  LogSupressScope(LogSupressScope&&)                 = delete;
  LogSupressScope& operator=(const LogSupressScope&) = delete;
  LogSupressScope& operator=(LogSupressScope&&)      = delete;

  ~LogSupressScope() {
    std::cout.rdbuf(_coutBuf);
    xpediteUnsupressLog();
  }

  std::string toString() {
    return _stream.str();
  }

private:

  std::ostringstream _stream;
  std::streambuf* _coutBuf;
};
