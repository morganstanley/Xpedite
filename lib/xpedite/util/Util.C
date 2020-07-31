///////////////////////////////////////////////////////////////////////////////
//
// A collection of data and utility methods to
//
//   1. Convert hex ascii byte pairs to 8 bit numbers
//   2. Pin threads to a given cpu core
//   3. List regular files at a given file system path
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include<xpedite/util/Util.H>
#include <xpedite/util/Errno.H>
#include<thread>
#include <sched.h>
#include <dirent.h>

namespace xpedite { namespace util {

  uint8_t atoiTable[1 << 8] {
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, +0, +1, +2, +3, +4, +5, +6, +7, +8, +9, 16, 16,
    16, 16, 16, 16, 16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 10, 11, 12,
    13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
  };

  void pinThread(std::thread::native_handle_type handle_, unsigned core_) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_, &cpuset);
    int rc = pthread_setaffinity_np(handle_, sizeof(cpu_set_t), &cpuset);
    if(rc != 0) {
      std::string errMsg;
      switch (rc) {
        case EFAULT:
          errMsg = "A supplied memory address was invalid";
          break;
        case EINVAL:
          errMsg = "supplied core was invalid";
          break;
        case ESRCH:
          errMsg = "thread not alive";
          break;
        default:
          errMsg = "unknown error";
          break;
      }
      std::ostringstream stream;
      stream << "xpedite - failed to pin thread [pthread_setaffinity_np error - " << rc << " | " << errMsg << "]";
      XpediteLogInfo << stream.str()<< XpediteLogEnd;
      throw std::runtime_error {stream.str()};
    }
  }

  std::vector<std::string> listFiles(const char* path_) {
    std::vector<std::string> files;
    if(DIR* dir = opendir(path_)) {
      while(auto* entry = readdir(dir)) {
        if(entry->d_type == DT_REG) {
          files.emplace_back(entry->d_name);
        }
      }
      closedir(dir);
    } else {
      util::Errno e;
      std::ostringstream stream;
      stream << "Failed to list dir \"" << path_ << "\" - " << e.asString();
      XpediteLogInfo << stream.str()<< XpediteLogEnd;
      throw std::runtime_error {stream.str()};
    }
    return files;
  }

}}
