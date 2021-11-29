///////////////////////////////////////////////////////////////////////////////
//
// DataSource - A collection of sample files in a profile
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <sstream>


namespace xpedite::txn {
  // Format of the sample file
  enum class SampleFileFormat
  {
    Binary,
    CommaSeperatedValues
  };

  enum class DataSourceType
  {
    Current,
    Benchmark
  };
}

inline std::string toString(xpedite::txn::SampleFileFormat fmt_) noexcept {
  using namespace xpedite::txn;
  switch(fmt_) {
    case SampleFileFormat::Binary:
      return "Binary";
    case SampleFileFormat::CommaSeperatedValues:
      return "CommaSeperatedValues";
  }
  return "UnKnown";
}

inline std::string toString(xpedite::txn::DataSourceType type_) noexcept {
  using namespace xpedite::txn;
  switch(type_) {
    case DataSourceType::Current:
      return "Current";
    case DataSourceType::Benchmark:
      return "Benchmark";
  }
  return "UnKnown";
}

namespace xpedite::txn {

  // Sample File for a thread
  class SampleFile
  {
    uint64_t _threadId;
    uint64_t _tlsAddr;
    std::string _path;
    SampleFileFormat _fmt;

    public:

    SampleFile(uint64_t threadId_, uint64_t tlsAddr_, std::string path_, SampleFileFormat fmt_)
      : _threadId {threadId_}, _tlsAddr {tlsAddr_}, _path {std::move(path_)}, _fmt {fmt_} {
    }

    uint64_t threadId()    const noexcept { return _threadId; }
    uint64_t tlsAddr()     const noexcept { return _tlsAddr;  }
    std::string path()     const noexcept { return _path;     }
    SampleFileFormat fmt() const noexcept { return _fmt;      }

    std::string toString() const {
      std::ostringstream stream;
      stream << ::toString(fmt()) << " sample file for thread id - " << threadId() 
        << " | tlsAddr - " << tlsAddr() << " | path - " << path();
      return stream.str();
    }
  };

  class DataSource
  {
    DataSourceType _type;
    std::string _name;
    std::string _appInfoPath;
    std::vector<SampleFile> _files;

    public:

    DataSource(DataSourceType type_, std::string appInfoPath_, std::vector<SampleFile> files_)
      : _type {type_}, _appInfoPath {std::move(appInfoPath_)}, _files {std::move(files_)} {
    }

    DataSourceType type()           const noexcept { return _type;        }
    std::string name()              const noexcept { return _name;        }
    std::string appInfoPath()       const noexcept { return _appInfoPath; }
    std::vector<SampleFile> files() const noexcept { return _files;       }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Data source:" << std::endl
        << "\t\t" << appInfoPath() << std::endl;
      for(const auto& file : files()) {
        stream << "\t\t" << file.toString() << std::endl;
      }
      return stream.str();
    }
  };
}

namespace std {
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::txn::SampleFile& file_) {
    stream_ << file_.toString();
    return stream_;
  }
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::txn::DataSource& dataSource_) {
    stream_ << dataSource_.toString();
    return stream_;
  }
}
