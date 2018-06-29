///////////////////////////////////////////////////////////////////////////////
//
// Provides classes to capture and consolidate stack traces.
//
// Trace - Captures stack trace of the calling thread.
//         Provides methods to convert traces to human friendly format
//
// ReentrantState - Container to store distinct stack traces.
//                  Duplicates are eliminated using trace origin as key
//                  Provides logic to track reentrancy and stack depth
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include "TlScopedDatum.H"
#include <xpedite/util/Util.H>
#include <fcntl.h>
#include <unistd.h>
#include <execinfo.h>
#include <array>
#include <iomanip>
#include <vector>
#include <map>
#include <sstream>
#include <cassert>

namespace xpedite { namespace intercept {

  static thread_local bool _traceMemoryOp;

  void enableMemoryOpTracing() {
    _traceMemoryOp = true;
  }

  void disableMemoryOpTracing() {
    _traceMemoryOp = {};
  }

  class Trace
  {
    std::array<void*, 128> _traceBuffer;
    int _frameCount;
    const char* _op;
    void* _addr;
    std::size_t _size;
    bool _reportOnceFlag;

    public:

    Trace(const char* op_, void* addr_, std::size_t size_)
      : _frameCount {}, _op {op_}, _addr {addr_}, _size {size_}, _reportOnceFlag {} {
      _frameCount = backtrace(_traceBuffer.data(), _traceBuffer.size());
    }

    void* origin() const noexcept {
      return _traceBuffer[2];
    }

    void report(std::ostringstream& stream_) {
      if(_reportOnceFlag) {
        return;
      }

      auto btSymbols = backtrace_symbols(_traceBuffer.data(), _frameCount);
      if(btSymbols) {
        stream_ << "--------------------xpedite trace (" << _op << ") - " << origin() << "--------------------" << std::endl;
        stream_ << "###"
                << "  tid: 0x" << util::gettid()
                << "  op: " << std::left << std::setfill(' ') << std::setw(7) << _op
                << "  mem: " << _addr;

        if(_size != static_cast<std::size_t>(-1)) {
          stream_ << "  size: " << _size;
        }

        for(int i=0; i<_frameCount; ++i) {
          stream_ << btSymbols[i] << std::endl;
        }
        free(btSymbols);
      }
      else {
        stream_ << "failed to trace memory op call from " << origin() << std::endl;
      }
      _reportOnceFlag = true;
    }
  };

  class ReentrantState
  {
    int _stackDepth;
    std::map<void*, Trace> _traces;
    
    public:

    ReentrantState()
      : _stackDepth {}, _traces {} {
    }

    void enter() noexcept {
      assert(_stackDepth >=0);
      ++_stackDepth;
    }

    void exit() noexcept {
      assert(_stackDepth >=1);
      --_stackDepth;
    }

    bool isNested() const noexcept {
      assert(_stackDepth >=0);
      return _stackDepth > 1;
    }

    int stackDepth() const noexcept {
      return _stackDepth;
    }

    void captureTrace(const char* op_, void* mem_, std::size_t size_) {
      Trace trace {op_, mem_, size_};
      if(_traces.find(trace.origin()) == _traces.end()) {
        _traces.emplace(trace.origin(), trace);
      }
    }

    std::string report() {
      std::ostringstream stream;
      for(auto& kvp : _traces) {
        auto& trace = kvp.second;
        trace.report(stream);
      }
      return stream.str();
    }
  };

  static thread_local ReentrantState reentrantState;

  class ReentrantScope
  {
    public:

    ReentrantScope() {
      reentrantState.enter();
    }

    ~ReentrantScope() {
      reentrantState.exit();
    }
  };

  void interceptOp(const char* op_, void* mem_, std::size_t size_) {
    if(!_traceMemoryOp) {
      return;
    }

    ReentrantScope guard {};
    if(!reentrantState.isNested()) {
      reentrantState.captureTrace(op_, mem_, size_);
    }
  }

  std::string reportMemoryOp() {
    return reentrantState.report();
  }
}}
