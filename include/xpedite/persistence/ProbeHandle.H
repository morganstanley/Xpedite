///////////////////////////////////////////////////////////////////////////////
//
// ProbeHandle - Handle to probe, combines probe info with ux probes
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/persistence/ProbeInfo.H>
#include <xpedite/ux/UxProbe.H>
#include <map>
#include <sstream>
#include <memory>

namespace xpedite::persistence {

  class ProbeHandle {

    using UxProbe = ux::UxProbe;
    using ProbeType = ux::ProbeType;

    UxProbe _uxProbe;
    const ProbeInfo* _probeInfo;
    std::string _probeName;

    ProbeType _type() const noexcept {
      if(_probeInfo->canBeginTxn()) {
        return ProbeType::TxnBeginProbe;
      } else if(_probeInfo->canSuspendTxn()) {
        return ProbeType::TxnSuspendProbe;
      } else if(_probeInfo->canResumeTxn()) {
        return ProbeType::TxnResumeProbe;
      }
      assert(_probeInfo->canEndTxn());
      return ProbeType::TxnEndProbe;
    }

    public:

    ProbeHandle(UxProbe uxProbe_, const ProbeInfo* probeInfo_)
      : _uxProbe {std::move(uxProbe_)}, _probeInfo {probeInfo_}, _probeName {_probeInfo->probeName()} {
    }

    const UxProbe& uxProbe()              const noexcept { return _uxProbe;                             }
    const ProbeInfo* probeInfo()          const noexcept { return _probeInfo;                           }
    const std::string& sysName()          const noexcept { return _probeName;                           }
    const std::string& getCanonicalName() const noexcept { return sysName();                            }
    bool isActive()                       const noexcept { return _probeInfo->isActive();               }
    bool isAnonymous()                    const noexcept { return false;                                }
    bool isAnchored()                     const noexcept { return false;                                }
    const char* probeName()               const noexcept { return _probeInfo->probeName();              }
    uint32_t probeNameSize()              const noexcept { return _probeInfo->probeNameSize();          }
    const char* fileName()                const noexcept { return _probeInfo->fileName();               }
    uint32_t fileNameSize()               const noexcept { return _probeInfo->fileNameSize();           }
    const char* functionName()            const noexcept { return _probeInfo->functionName();           }
    uint32_t functionNameSize()           const noexcept { return _probeInfo->functionNameSize();       }
    uint32_t lineNo()                     const noexcept { return _probeInfo->lineNo();                 }
    bool canBeginTxn()                    const noexcept { return type() == ProbeType::TxnBeginProbe;   }
    bool canSuspendTxn()                  const noexcept { return type() == ProbeType::TxnSuspendProbe; }
    bool canResumeTxn()                   const noexcept { return type() == ProbeType::TxnResumeProbe;  }
    bool canEndTxn()                      const noexcept { return type() == ProbeType::TxnEndProbe;     }

    ProbeType type() const noexcept {
      return _uxProbe.type() != ProbeType::Invalid ? _uxProbe.type() : _type();
    }

    const std::string& name() const noexcept {
      return !_uxProbe.name().empty() ? _uxProbe.name() : sysName(); 
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Probe [name : " << name() << ", type : " << ux::toString(type()) << "]";
      return stream.str();
    }
  };

  struct ProbeHandles : public std::enable_shared_from_this<ProbeHandles>
  {
    using UxProbe = ux::UxProbe;
    using Map = std::map<const void*, ProbeHandle>;

    ProbeHandles(const ProbeInfoMapPtr& probeInfoMap_, std::vector<UxProbe> uxProbes_)
      : _map {} {
      std::unordered_map<std::string, UxProbe*> uxProbes;
      std::for_each(uxProbes_.begin(), uxProbes_.end(), [&uxProbes](UxProbe& p_) {
        uxProbes.emplace(p_.sysName(), &p_);
      });
      auto& map = probeInfoMap_->data();
      std::for_each(map.begin(), map.end(), [&uxProbes, this](const ProbeInfoMap::value_type& kvp_) {
        UxProbe uxProbe {};
        if(auto it = uxProbes.find(kvp_.second->probeName()); it != uxProbes.end()) {
          // make a copy (NOT move), since more than one probe info can map to same ux probe
          uxProbe = *it->second;
        }
        _map.emplace(kvp_.second->recorderReturnSite(), ProbeHandle {std::move(uxProbe), kvp_.second});
      });
    }
    
    std::shared_ptr<ProbeHandles> ptr() {
      return shared_from_this();
    }

    const ProbeHandle* find(const void* recorderReturnSite_) const noexcept {
      if(auto it = _map.find(recorderReturnSite_); it != _map.end()) {
        return &it->second;
      }
      return {};
    }

    const Map& data() const noexcept {
      return _map;
    }

    std::string toString() const {
      std::ostringstream stream;
      for(auto& [_, probeHandle] : _map) {
        stream << probeHandle.toString() << std::endl;
      }
      return stream.str();
    }

    private:

    Map _map;
  };

  using ProbeHandlesPtr = std::shared_ptr<ProbeHandles>;
}

namespace std {
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::persistence::ProbeHandle& probe_) {
    stream_ << probe_.toString();
    return stream_;
  }
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::persistence::ProbeHandles& probes_) {
    stream_ << probes_.toString();
    return stream_;
  }
}
