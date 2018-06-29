///////////////////////////////////////////////////////////////////////////////
//
// Provides implementation for storing and locating information about call sites
// in a trager application.
//
// CallSiteInfo - Stores the address, attribtues and id of a call site
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

namespace xpedite { namespace framework {

  class CallSiteInfo
  {
    const void* _callSite;
    const probes::CallSiteAttr _attr;
    const uint32_t _id;

    public:

    CallSiteInfo(const void* callSite_, probes::CallSiteAttr attr_, uint32_t id_)
      : _callSite {callSite_}, _attr {attr_}, _id {id_} {
    }

    const void* callSite()    const noexcept { 
      return _callSite;
    }

    uint32_t id()             const noexcept { return _id;                   }
    bool canStoreData()       const noexcept { return _attr.canStoreData();  }
    bool canBeginTxn()        const noexcept { return _attr.canBeginTxn();   }
    bool canSuspendTxn()      const noexcept { return _attr.canSuspendTxn(); }
    bool canResumeTxn()       const noexcept { return _attr.canResumeTxn();  }
    bool canEndTxn()          const noexcept { return _attr.canEndTxn();     }

    std::string toString() const {
      std::ostringstream os;
      os << "CallSite - " <<  callSite() << " | id - " << _id << " | " << _attr.toString();
      return os.str();
    }

  } __attribute__((packed));


  class CallSiteMap
  {
    std::unordered_map<const void*, const CallSiteInfo> _map;

    public:

    void add(const CallSiteInfo callSiteInfo_) {
      _map.emplace(callSiteInfo_.callSite(), callSiteInfo_);
    }

    const CallSiteInfo* locateInfo(const void* callSite_) const noexcept {
      auto it = _map.find(callSite_);
      if(it != _map.end()) {
        return &(it->second);
      }
      return {};
    }

    std::string toString() const {
      std::ostringstream os;
      for(auto& kvp : _map) {
        os << kvp.second.toString() << std::endl;
      }
      return os.str();
    }

  };

}}
