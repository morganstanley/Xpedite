///////////////////////////////////////////////////////////////////////////////
//
// Utility methods to build a list of threads in a process
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/util/ThreadInfo.H>
#include <dirent.h>
#include <stdlib.h>
#include <stdexcept>
#include <sstream>

namespace xpedite { namespace util {

  std::vector<pid_t> getChildren(pid_t pid_) {
    std::ostringstream stream;
    stream << "/proc/" << pid_ << "/task";
    auto taskFile = stream.str();
    DIR *proc_dir = opendir(taskFile.c_str());

    std::vector<pid_t> children;
    if(proc_dir) {
      dirent *entry {};
      while ((entry = readdir(proc_dir))) {
        if(entry->d_name[0] == '.') {
          continue;
        }
        pid_t tid = atoi(entry->d_name);
        children.emplace_back(tid);
      }
      closedir(proc_dir);
    }
    else {
      std::ostringstream stream;
      stream << "Failed to locate process " << pid_;
      throw std::runtime_error {stream.str()};
    }
    return children;
  }
}}
