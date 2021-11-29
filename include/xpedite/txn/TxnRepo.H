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
#include <xpedite/txn/TxnCollection.H>
#include <xpedite/log/Log.H>
#include <string>
#include <map>
#include <optional>
#include <memory>
#include <sstream>

namespace xpedite::txn {

  struct TxnRepo : public std::enable_shared_from_this<TxnRepo>
  {
    public:
    using Benchmarks = std::map<std::string, TxnCollectionPtr>;

    TxnRepo(const TxnRepo&)            = delete;
    TxnRepo& operator=(const TxnRepo&) = delete;
    TxnRepo(TxnRepo&&)                 = delete;
    TxnRepo& operator= (TxnRepo&&)     = delete;

		TxnRepo()
			: _current {}, _benchmarks {} {
		}

		~TxnRepo() {
      XpediteLogInfo << "TxnRepo destroy" << XpediteLogEnd;
    }

    std::shared_ptr<TxnRepo> ptr() {
      return shared_from_this();
    }


    // Adds transaction collection for a benchmark to this repository
    bool addBenchmark(TxnCollectionPtr collection_) {
      if(_current && *_current == *collection_) {
        XpediteLogInfo << "attempting to add current transaction collection " << *collection_ << " as benchmark" << XpediteLogEnd;
        return {};
      }
      XpediteLogInfo << "adding benchmark collection " << *collection_ << XpediteLogEnd;
      auto rc = _benchmarks.emplace(collection_->name(), std::move(collection_));
      return rc.second;
    }

    // Adds transaction collection for current profile session to this repository
    bool setCurrent(TxnCollectionPtr collection_) {
      if(_current) {
        XpediteLogInfo << "Attempt to register multiple current collections - Repository already has collection "
          << *_current << "marked current" << XpediteLogEnd;
        return {};
      }
      XpediteLogInfo << "adding current collection " << *collection_ << XpediteLogEnd;
      collection_->makeCurrent();
      _current = std::move(collection_);
      return true;
    }

    TxnCollectionPtr getCurrent() const noexcept {
      return _current;
    }

    bool hasBenchmarks() const noexcept {
      return !_benchmarks.empty();
    }

    const Benchmarks& getBenchmarks() const noexcept {
      return _benchmarks;
    }

    TxnCollectionPtr getBenchmark(std::string name_) const {
      if(auto it = _benchmarks.find(name_); it != _benchmarks.end()) {
        return it->second;
      }
      throw std::runtime_error {"failed to locate transactions for benchmark - " + name_};
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "TxnRepo {hasCurrent : " << (_current ? "true" : "false") 
        << ", benchmarkCount : " << _benchmarks.size() << "}";
      return stream.str();
    }

    private:
    TxnCollectionPtr _current;
    Benchmarks _benchmarks;
  };

  using TxnRepoPtr = std::shared_ptr<TxnRepo>;

}
