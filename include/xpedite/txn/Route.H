///////////////////////////////////////////////////////////////////////////////
//
// Route object
// 
// A Route is a ordered sequence of probes, that got hit in a transaction.
// The ordering of probes in a route matches the flow of control (program order).
// Routes are used for aggregating and conflating transaction objects.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/persistence/ProbeHandle.H>
#include <sstream>

namespace xpedite { namespace txn {

  class RouteFactory;
  struct Route
  {
    using ProbeHandle = persistence::ProbeHandle;
    using Probes = std::vector<const ProbeHandle*>;

    const Probes& probes() const noexcept { return _probes;        }
    size_t size()          const noexcept { return _probes.size(); }
    std::string key()      const noexcept { return _key;           }

    bool hasProbe(const std::string& probeName_) {
      return _probesMap.find(probeName_) != _probesMap.end();
    }

    const std::vector<size_t>& indices(const std::string& probeName_) {
      static const std::vector<size_t> empty {};
      if(auto it = _probesMap.find(probeName_); it != _probesMap.end()) {
        return it->second;
      }
      return empty;
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Route [";
      for(size_t i=0; i<_probes.size(); ++i) {
        stream << "\t" << *_probes[i] << (i < _probes.size()-1 ? "," : "") << std::endl;
      }
      stream << "]";
      return stream.str();
    }

    private:
    
    Route(Probes probes_, std::string key_)
      : _probes {std::move(probes_)}, _probesMap {}, _key {std::move(key_)} {
      for(size_t i=0; i<_probes.size(); ++i) {
        _probesMap[_probes[i]->probeName()].emplace_back(i);
      }
    }

    friend class RouteFactory;

    Probes _probes;
    std::unordered_map<std::string, std::vector<size_t>> _probesMap;
    std::string _key;
  };

  inline bool operator==(const Route& lhs_, const Route& rhs_) {
    return lhs_.key() == rhs_.key();
  }

  class RouteFactory
  {
    using ProbeHandle = persistence::ProbeHandle;

    std::unordered_map<std::string, Route> _routes;

    inline static RouteFactory* _instance;

    RouteFactory() = default;

    public:

    Route* makeRoute(const std::vector<Counter>& counters_) {
      size_t keySize {};
      for(const auto& counter : counters_) {
        keySize += counter.probe()->probeNameSize();
      }
      std::string key;
      key.reserve(keySize);
      for(const auto& counter : counters_) {
        auto* probe {counter.probe()};
        key.append(probe->probeName(), probe->probeNameSize()-1).append("|");
      }
      if(auto it = _routes.find(key); it != _routes.end()) {
        return &it->second;
      }
      std::vector<const ProbeHandle*> probes;
      probes.reserve(counters_.size());
      for(const auto& counter : counters_) {
        probes.emplace_back(counter.probe());
      }
      Route route {std::move(probes), key};
      auto it = _routes.emplace(std::move(key), std::move(route));
      assert(it.second);
      return &it.first->second;
    }

    static RouteFactory& get() {
      if(!_instance) {
        _instance = new RouteFactory {};
      }
      return *_instance;
    }
  };

}}

namespace std {
  template <> struct hash<xpedite::txn::Route>
  {
    size_t operator()(const xpedite::txn::Route& route_) const
    {
      return std::hash<std::string>{}(route_.key());
    }
  };

  template <> struct equal_to<xpedite::txn::Route>
  {
    size_t operator()(const xpedite::txn::Route& lhs_, const xpedite::txn::Route& rhs_) const
    {
      return std::equal_to<std::string>{}(lhs_.key(), rhs_.key());
    }
  };

  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::txn::Route& route_) {
    stream_ << route_.toString();
    return stream_;
  }
}
