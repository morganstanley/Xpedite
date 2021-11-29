///////////////////////////////////////////////////////////////////////////////
//
// Transaction and TransactionCollection
// 
// Transaction is any functionality in the program, that is a meaningful
// target for profiling and optimisations.
// 
// A transaction stores data (timestamps and h/w counters) from a collection
// of probes, that got hit, during program execution to achieve the functionality.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/txn/Counter.H>
#include <xpedite/txn/Route.H>
#include <list>
#include <sstream>
#include <memory>

namespace xpedite::txn {

  using TxnId = uint64_t;

  class Txn;
  using TxnPtr = std::shared_ptr<Txn>;
  using Txns = std::map<TxnId, TxnPtr>;


  class Txn : public std::enable_shared_from_this<Txn>
  {
    TxnId _id;
    std::vector<Counter> _counters;
    Route* _route;
    bool _hasEndProbe;
    bool _isIntact;

    public:

    Txn(TxnId txnId_, Counter counter_)
      : _id {txnId_}, _counters {counter_}, _route {}, _hasEndProbe {}, _isIntact {true} {
    }

    std::shared_ptr<Txn> ptr() {
      return shared_from_this();
    }

    void setId(TxnId id_) noexcept {
      _id = id_;
    }

    TxnId id()                             const noexcept { return _id;              }
    size_t size()                          const noexcept { return _counters.size(); }
    bool hasEndProbe()                     const noexcept { return _hasEndProbe;     }
    const std::vector<Counter>& counters() const noexcept { return _counters;        }
    Route* route()                         const noexcept { return _route;           }

    void add(Counter counter_) {
      auto* probe {counter_.probe()};
      _hasEndProbe |= probe->canEndTxn() || probe->canSuspendTxn();
      _isIntact &= _counters.back().tsc() < counter_.tsc();
      _counters.emplace_back(counter_);
    }

    void join(const TxnPtr& txn_) {
      for(auto& counter : txn_->_counters) {
        add(counter);
      }
    }

    void finalize(Route* route_) {
      _route = route_;
    }

    bool hasProbe(const std::string& probeName_) {
      assert(_route);
      return _route->hasProbe(probeName_);
    }

    const Counter* getCounterForProbe(const std::string& probeName_, uint32_t index=0) {
      assert(_route);
      auto& indices = _route->indices(probeName_);
      if(index >= indices.size()) {
        return {};
      }
      return &_counters[indices[index]];
    }

    uint64_t getElapsedTsc() const noexcept {
      return _counters.back().tsc() - _counters.front().tsc();
    }

    const Counter& operator[](size_t index_) const noexcept {
      return _counters[index_];
    }

    explicit operator bool() {
      return _hasEndProbe && _isIntact;
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Txn {id : " << id() << ", route : " << *_route << "}";
      return stream.str();
    }
  };
}

namespace std {
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::txn::Txn& txn_) {
    stream_ << txn_.toString();
    return stream_;
  }
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::txn::Txns& txns_) {
    for(auto& [id, txn] : txns_) {
      stream_ << txn << std::endl;
    }
    return stream_;
  }
}
