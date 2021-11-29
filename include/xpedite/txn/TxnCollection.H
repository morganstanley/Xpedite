///////////////////////////////////////////////////////////////////////////////
//
// Transaction repository
// 
// TxnRepo defines a container to store transaction collection from multiple runs.
// The repo is used to store profile data from multiple benchmarks along with current run.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/persistence/SamplesLoader.H>
#include <xpedite/txn/Txn.H>
#include <xpedite/persistence/ProfileInfo.H>

#include <sstream>
#include <string>
#include <vector>
#include <memory>

namespace xpedite::txn {

  class TxnSubCollection;

  class TxnCollection : public std::enable_shared_from_this<TxnCollection>
  {
    using ProfileInfo = persistence::ProfileInfo;
    using SamplesLoader = persistence::SamplesLoader;

    std::string _name;
    ProfileInfo _profileInfo;
    std::vector<SamplesLoader> _loaders;
    Txns _txns;
    bool _isCurrent;

    public:

    TxnCollection(std::string name_, ProfileInfo profileInfo_, std::vector<SamplesLoader>&& loaders_, Txns&& txns_)
      : _name {std::move(name_)}, _profileInfo {std::move(profileInfo_)},
        _loaders {std::move(loaders_)}, _txns {std::move(txns_)} {
    }

    TxnCollection(const TxnCollection&)            = delete;
    TxnCollection& operator=(const TxnCollection&) = delete;
    TxnCollection(TxnCollection&&)                 = delete;
    TxnCollection& operator= (TxnCollection&&)     = delete;

    const std::string& name() const noexcept { return _name;                       }
    const auto* cpuInfo()     const noexcept { return _profileInfo.cpuInfo();      }
    auto probes()             const noexcept { return _profileInfo.probes();       }
    auto events()             const noexcept { return _profileInfo.events();       }
    auto topdownNodes()       const noexcept { return _profileInfo.topdownNodes(); }
    auto metrics()            const noexcept { return _profileInfo.metrics();      }
    const Txns& data()        const noexcept { return _txns;                       }
    bool isCurrent()          const noexcept { return _isCurrent;                  }

    TxnSubCollection getSubCollection();

    void makeCurrent() noexcept {
      _isCurrent = true;
    }
    
    std::shared_ptr<TxnCollection> ptr() {
      return shared_from_this();
    }

    explicit operator bool() const noexcept {
      return !_txns.empty();
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "TxnCollection {\nname : " << name() << std::endl;
      for(auto& [id, txn] : _txns) {
        stream << "\t\t" << *txn << std::endl;
      }
      stream << std::endl << "}" << std::endl ;
      return stream.str();
    }
  };

  using TxnCollectionPtr = std::shared_ptr<TxnCollection>;

  class TxnSubCollection
  {
    TxnCollectionPtr _src;
    Txns _txns;

    public:

    explicit TxnSubCollection(TxnCollectionPtr src_)
      : _src {std::move(src_)}, _txns {} {
    }

    const std::string& name() const noexcept { return _src->name();         }
    const auto* cpuInfo()     const noexcept { return _src->cpuInfo();      }
    auto probes()             const noexcept { return _src->probes();       }
    auto events()             const noexcept { return _src->events();       }
    auto topdownNodes()       const noexcept { return _src->topdownNodes(); }
    auto metrics()            const noexcept { return _src->metrics();      }
    const Txns& data()        const noexcept { return _txns;                }
    size_t size()             const noexcept { return _txns.size();         }

    void append(TxnPtr tnx_) {
      auto id = tnx_->id();
      _txns.emplace(id, std::move(tnx_));
    }

    inline TxnSubCollection getSubCollection() {
      return TxnSubCollection {_src};
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "TxnCollection {\nname : " << name() << std::endl;
      for(auto& [id, txn] : _txns) {
        stream << "\t\t" << *txn << std::endl;
      }
      stream << std::endl << "}" << std::endl ;
      return stream.str();
    }
  };

  inline TxnSubCollection TxnCollection::getSubCollection() {
    return TxnSubCollection {ptr()};
  }

  inline bool operator==(const TxnCollection& lhs_, const TxnCollection& rhs_) {
    return lhs_.name() == rhs_.name();
  }
}

namespace std {
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::txn::TxnCollection& collection_) {
    stream_ << collection_.toString();
    return stream_;
  }
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::txn::TxnSubCollection& subCollection_) {
    stream_ << subCollection_.toString();
    return stream_;
  }
}
