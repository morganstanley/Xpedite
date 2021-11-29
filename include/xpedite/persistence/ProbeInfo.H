///////////////////////////////////////////////////////////////////////////////
//
// Provides implementation for storing and locating information about call sites
// in a trager application.
//
// ProbeInfo - Stores the address, attribtues and id of a call site
//
// CallSiteMap - A collection of call sites in a target application
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/probes/CallSite.H>
#include <xpedite/persistence/SegmentHeader.H>
#include <unordered_map>
#include <sstream>
#include <cstring>
#include <cassert>
#include <sstream>

namespace xpedite::persistence {

  template<SegmentHeader::Type, typename> class Segment;

  class ProbeInfo
  {
    friend class FileHeader;
    template<SegmentHeader::Type, typename> friend class Segment;

    const void* _recorderReturnSite;
    probes::CallSiteAttr _attr;
    uint32_t _id;
    uint32_t  _probeNameOffset;
    uint32_t  _probeNameSize;
    uint32_t _fileNameOffset;
    uint32_t _fileNameSize;
    uint32_t _functionNameOffset;
    uint32_t _functionNameSize;
    uint32_t _lineNo;
    uint32_t _size;
    char _data[0];

    ProbeInfo(const void* recorderReturnSite_, probes::CallSiteAttr attr_, uint32_t id_, std::string_view probeName_,
        std::string_view fileName_, std::string_view functionName_, uint32_t lineNo_)
      : _recorderReturnSite {recorderReturnSite_}, _attr {attr_}, _id {id_}, _lineNo {lineNo_}, _size {} {

      _probeNameOffset = _size;
      _probeNameSize = probeName_.size();
      memcpy(_data+_size, probeName_.data(), probeName_.size());
      _size += probeName_.size();

      _fileNameOffset  = _size;
      _fileNameSize = fileName_.size();
      memcpy(_data+_size, fileName_.data(), fileName_.size());
      _size += fileName_.size();

      _functionNameOffset  = _size;
      _functionNameSize = functionName_.size();
      memcpy(_data+_size, functionName_.data(), functionName_.size());
      _size += functionName_.size();
    }

    ProbeInfo (const ProbeInfo&)            = delete;
    ProbeInfo& operator=(const ProbeInfo&)  = delete;
    ProbeInfo (ProbeInfo&&)                 = delete;
    ProbeInfo& operator=(ProbeInfo&&)       = delete;

    public:

    const void* recorderReturnSite() const noexcept { return _recorderReturnSite;          }
    uint32_t id()                    const noexcept { return _id;                          }
    bool isActive()                  const noexcept { return _attr.isActive();             }
    bool canStoreData()              const noexcept { return _attr.canStoreData();         }
    bool canBeginTxn()               const noexcept { return _attr.canBeginTxn();          }
    bool canSuspendTxn()             const noexcept { return _attr.canSuspendTxn();        }
    bool canResumeTxn()              const noexcept { return _attr.canResumeTxn();         }
    bool canEndTxn()                 const noexcept { return _attr.canEndTxn();            }
    const char* probeName()          const noexcept { return _data + _probeNameOffset;     }
    uint32_t probeNameSize()         const noexcept { return _probeNameSize;               }
    const char* fileName()           const noexcept { return _data + _fileNameOffset;      }
    uint32_t fileNameSize()          const noexcept { return _fileNameSize;                }
    const char* functionName()       const noexcept { return _data + _functionNameOffset;  }
    uint32_t  functionNameSize()     const noexcept { return _functionNameSize;            }
    uint32_t lineNo()                const noexcept { return _lineNo;                      }
    size_t size()                    const noexcept { return sizeof(ProbeInfo) + _size;    }

    std::string toString() const {
      std::ostringstream os;
      os << "RecorderReturnSite - " <<  recorderReturnSite() << " | id - " << _id << " | " << _attr.toString()
        << " | probe name - " << probeName() << " | file name - " << fileName()
        << " | line no - " << _lineNo << " | function - " << functionName();
      return os.str();
    }

  } __attribute__((packed));

  class ProbeInfoMap
  {
    using Map = std::unordered_map<const void*, const ProbeInfo*>;
    Map _map;

    public:

    using value_type = Map::value_type;

    void add(const ProbeInfo* probeInfo_) {
      _map.emplace(probeInfo_->recorderReturnSite(), probeInfo_);
    }

    const ProbeInfo* locateInfo(const void* recorderReturnSite_) const noexcept {
      auto it = _map.find(recorderReturnSite_);
      if(it != _map.end()) {
        return it->second;
      }
      return {};
    }

    const Map& data() const noexcept {
      return _map;
    }

    std::string toString() const {
      std::ostringstream os;
      for(auto& kvp : _map) {
        os << kvp.second->toString() << std::endl;
      }
      return os.str();
    }
  };

  using ProbeInfoMapPtr = std::shared_ptr<ProbeInfoMap>;

}

namespace std {
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::persistence::ProbeInfo& probeInfo_) {
    stream_ << probeInfo_.toString();
    return stream_;
  }
}

