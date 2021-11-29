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
#include <unordered_map>
#include <sstream>
#include <cstring>
#include <cassert>

namespace xpedite { namespace framework {

  struct Name {
    // null terminated c-string
    const char* _data;

    // size of the string including the null-char
    uint32_t _size;
  };

  class FileHeader;

  enum class ProbeType {
    Invalid = 0,
    TxnBeginProbe,
    TxnSuspendProbe,
    TxnResumeProbe,
    TxnEndProbe
  };

  class ProbeInfo
  {
    friend class FileHeader;
    const void* _callSite;
    probes::CallSiteAttr _attr;
    uint32_t _id;
    uint32_t  _probeNameOffset;
    uint32_t _fileNameOffset;
    uint32_t _functionNameOffset;
    uint32_t _lineNo;
    uint32_t _size;
    char _data[0];

    ProbeInfo(const void* callSite_, probes::CallSiteAttr attr_, uint32_t id_, Name probeName_,
        Name fileName_, Name functionName_, uint32_t lineNo_)
      : _callSite {callSite_}, _attr {attr_}, _id {id_}, _lineNo {lineNo_}, _size {} {

      _probeNameOffset = _size;
      memcpy(_data+_size, probeName_._data, probeName_._size);
      _size += probeName_._size;

      _fileNameOffset  = _size;
      memcpy(_data+_size, fileName_._data, fileName_._size);
      _size += fileName_._size;

      _functionNameOffset  = _size;
      memcpy(_data+_size, functionName_._data, functionName_._size);
      _size += functionName_._size;
    }

    ProbeInfo (const ProbeInfo&)            = delete;
    ProbeInfo& operator=(const ProbeInfo&)  = delete;
    ProbeInfo (ProbeInfo&&)                 = delete;
    ProbeInfo& operator=(ProbeInfo&&)       = delete;

    public:

    const void* callSite()    const noexcept { 
      return _callSite;
    }

    uint32_t id()              const noexcept { return _id;                          }
    bool isActive()            const noexcept { return _attr.isActive();             }
    bool canStoreData()        const noexcept { return _attr.canStoreData();         }
    bool canBeginTxn()         const noexcept { return _attr.canBeginTxn();          }
    bool canSuspendTxn()       const noexcept { return _attr.canSuspendTxn();        }
    bool canResumeTxn()        const noexcept { return _attr.canResumeTxn();         }
    bool canEndTxn()           const noexcept { return _attr.canEndTxn();            }
    const char* probeName()    const noexcept { return _data + _probeNameOffset;     }
    const char* fileName()     const noexcept { return _data + _fileNameOffset;      }
    const char* functionName() const noexcept { return _data + _functionNameOffset;  }
    uint32_t lineNo()          const noexcept { return _lineNo;                      }
    size_t size()              const noexcept { return sizeof(ProbeInfo) + _size; }

    ProbeType type() const noexcept {
      if(canBeginTxn()) {
        return ProbeType::TxnBeginProbe;
      } else if(canSuspendTxn()) {
        return ProbeType::TxnSuspendProbe;
      } else if(canResumeTxn()) {
        return ProbeType::TxnResumeProbe;
      }
      assert(canEndTxn());
      return ProbeType::TxnEndProbe;
    }

    std::string toString() const {
      std::ostringstream os;
      os << "CallSite - " <<  callSite() << " | id - " << _id << " | " << _attr.toString()
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
      _map.emplace(probeInfo_->callSite(), probeInfo_);
    }

    const ProbeInfo* locateInfo(const void* callSite_) const noexcept {
      auto it = _map.find(callSite_);
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

}}
