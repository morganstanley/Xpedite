///////////////////////////////////////////////////////////////////////////////
//
// Transaction Loader
//
// TxnLoader provides functionality to build transactions from a sequence of probes.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/persistence/ProbeInfo.H>
#include <xpedite/probes/Sample.H>
#include <xpedite/txn/Counter.H>
#include <xpedite/txn/Route.H>
#include <xpedite/txn/Txn.H>
#include <xpedite/txn/TxnFragments.H>
#include <vector>
#include <memory>

namespace xpedite { namespace txn {

  enum class ReturnCode {
    SUCCESS,
    SAMPLE_NOT_MATCHING_PROBE
  };

  class TxnLoader
  {
    using ProbeHandlesPtr = persistence::ProbeHandlesPtr;
    using ProbeHandle = persistence::ProbeHandle;
    using Sample = probes::Sample;

    TxnFragments _txnFragments;
    ProbeHandlesPtr _probes;
    Txns _txns;
    TxnPtr _currentTxn;
    std::vector<Counter> _ephemeralCounters;
    std::optional<LinkId> _resumeId;

    uint64_t _threadId;
    uint64_t _tlsAddr;
    TxnId  _nextTxnId;
    size_t _sampleCount;
    size_t _invalidSampleCount;
    size_t _nonTxnSampleCount;
    size_t _compromisedTxnCount;

    TxnPtr makeTxn(Counter counter_);
    void finalizeCurrentTxn(std::optional<LinkId> suspendId_ = {});

    public:

    explicit TxnLoader(ProbeHandlesPtr probes_)
      : _txnFragments {}, _probes {probes_}, _txns {}, _currentTxn {},
        _ephemeralCounters {}, _resumeId {}, _threadId {}, _tlsAddr {}, _nextTxnId {}, _sampleCount {},
        _invalidSampleCount {}, _nonTxnSampleCount {}, _compromisedTxnCount {} {
    }

    size_t txnCount() const noexcept { return _txns.size();     }
    Txns&& moveTxns() noexcept       { return std::move(_txns); }

    size_t sampleCount()         const noexcept { return _sampleCount;         }
    size_t invalidSampleCount()  const noexcept { return _invalidSampleCount;  }
    size_t nonTxnSampleCount()   const noexcept { return _nonTxnSampleCount;   }
    size_t compromisedTxnCount() const noexcept { return _compromisedTxnCount; }

    void beginCollection() {
    }

    void beginLoad(uint64_t threadId_, uint64_t tlsAddr_) {
      _threadId = threadId_;
      _tlsAddr = tlsAddr_;
    }

    ReturnCode load(const Sample* sample_);

    void endLoad() {
      finalizeCurrentTxn();
    }

    void endCollection() {
      auto txns = _txnFragments.join(_nextTxnId);
      for(auto& [txnId, txn] : txns) {
        _txns.emplace(txnId, std::move(txn));
      }
    }
  };

}}
