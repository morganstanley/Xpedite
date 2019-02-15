///////////////////////////////////////////////////////////////////////////////
//
// A utility class to control storage for xpedite samples data
//
// The storage manager keeps track of current memory/file system consumption.
// It also provides methods to build file system paths for different data files
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include "StorageMgr.H"
#include <xpedite/util/Util.H>
#include <xpedite/util/Errno.H>
#include <sstream>
#include <cstring>
#include <cstdio>

namespace xpedite { namespace framework {

  const char* SAMPLES_DIR_PATH {"/dev/shm/"};

  const char* SAMPLES_FILE_SUFFIX {".data"};

  size_t SAMPLES_FILE_SUFFIX_LEN {strlen(SAMPLES_FILE_SUFFIX)};

  std::string StorageMgr::buildSamplesFilePrefix() {
    std::ostringstream stream;
    stream << "xpedite-" << util::getProcessName();
    return stream.str();
  }

  std::string StorageMgr::buildSamplesFileTemplate() {
    std::ostringstream stream;
    stream << SAMPLES_DIR_PATH << buildSamplesFilePrefix()
      << "-"  << time(nullptr) << "-*" << SAMPLES_FILE_SUFFIX;
    return stream.str();
  }

  void StorageMgr::reset() {
    auto filePrefix = buildSamplesFilePrefix();
    auto files = util::listFiles(SAMPLES_DIR_PATH);
    int fileCount {};
    std::ostringstream stream;
    stream << "Xpedite purging old sample files ";
    for(auto& file : files) {
      if(file.find(filePrefix) == 0 &&
        file.rfind(SAMPLES_FILE_SUFFIX) == file.size() - SAMPLES_FILE_SUFFIX_LEN) {
        auto path = SAMPLES_DIR_PATH + file;
        stream << "\n\t->\t " << path;
        if(remove(path.c_str())) {
          util::Errno e;
          stream << " - [" << e.asString() << "]";
        } else {
          stream << " - [DELETED]";
        }
        ++fileCount;
      }
    }
   XpediteLogInfo << stream.str() << "\nremoved " << fileCount
     << " out of " << files.size() << " file(s)" << XpediteLogEnd;
  }

}}
