///////////////////////////////////////////////////////////////////////////////
//
// Utility methods to build a list of threads in a process
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <unistd.h>
#include <sys/types.h>
#include <vector>

namespace xpedite { namespace util {

  std::vector<pid_t> getChildren(pid_t pid_);

  inline std::vector<pid_t> getChildren() {
    return getChildren(getpid());
  }
}}
