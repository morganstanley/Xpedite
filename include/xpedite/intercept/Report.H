///////////////////////////////////////////////////////////////////////////////
//
// Api to trace memory allocation operations.
//
// Provide instrumented wrappers for common memory allocation methods.
//
// Also implements RAII scoped object (MemOpReportScope) to report 
// stack traces at the end of the scope.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <cstddef>
#include <iostream>

namespace xpedite { namespace intercept {

  void enableMemoryOpTracing();

  void disableMemoryOpTracing();

  std::string reportMemoryOp();

  struct MemOpReportScope
  {
    MemOpReportScope() {
      enableMemoryOpTracing();
    }

    ~MemOpReportScope() {
      disableMemoryOpTracing();
      std::cout << reportMemoryOp();
    }

    MemOpReportScope(const MemOpReportScope&) = delete;
    MemOpReportScope& operator=(const MemOpReportScope&) = delete;
    MemOpReportScope(MemOpReportScope&&) = delete;
    MemOpReportScope& operator=(MemOpReportScope&&) = delete;
  };

}}
