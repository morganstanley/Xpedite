///////////////////////////////////////////////////////////////////////////////
//
// Transaction Loader
//
// TxnLoader provides functionality to build transactions from a sequence of probes.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/txn/TxnLoader.H>

namespace xpedite { namespace txn {

  TxnPtr TxnLoader::makeTxn(Counter counter_) {
    if(counter_.probe()->canResumeTxn()) {
      auto[tlsAddr, tsc] = counter_.data();
      _resumeId = LinkId {tlsAddr, tsc};
    } else {
      _resumeId = {};
    }
    return std::make_shared<Txn>(++_nextTxnId, std::move(counter_));
  }

  void TxnLoader::finalizeCurrentTxn(std::optional<LinkId> suspendId_) {
    if(!_currentTxn || !*_currentTxn) {
      _currentTxn.reset();
      return;
    }
    if(_resumeId || suspendId_) {
      _txnFragments.addTxn(std::move(_currentTxn), _resumeId, suspendId_);
    } else if(_currentTxn->hasEndProbe()) {
      auto* route = RouteFactory::get().makeRoute(_currentTxn->counters());
      _currentTxn->finalize(route);
      auto id = _currentTxn->id();
      _txns.emplace(id, std::move(_currentTxn));
    } else {
      ++_compromisedTxnCount;
    }
    _currentTxn.reset();
  }

  ReturnCode TxnLoader::load(const Sample* sample_) {
    ++_sampleCount;
    const ProbeHandle* probe {_probes->find(sample_->returnSite())};
    if(!probe) {
      ++_invalidSampleCount;
      return ReturnCode::SAMPLE_NOT_MATCHING_PROBE;
    }

    Counter counter {_threadId, sample_, probe};
    if(_currentTxn) {
      if(probe->canBeginTxn() || probe->canResumeTxn()) {
        if(_currentTxn->hasEndProbe() || probe->canResumeTxn()) {
          if(!_ephemeralCounters.empty()) {
            _nonTxnSampleCount += _ephemeralCounters.size();
            _ephemeralCounters.clear();
          }
          finalizeCurrentTxn();
          _currentTxn = makeTxn(std::move(counter));
        } else {
          _currentTxn->add(counter);
        }
      } else if(probe->canEndTxn() || probe->canSuspendTxn()) {
        if(!_ephemeralCounters.empty()) {
          for(auto& counter : _ephemeralCounters) {
            _currentTxn->add(std::move(counter));
          }
          _ephemeralCounters.clear();
        }
        _currentTxn->add(counter);
        if(probe->canSuspendTxn()){
          finalizeCurrentTxn(LinkId {_tlsAddr, counter.tsc()});
        }
      } else {
        if(_currentTxn->hasEndProbe()) {
          _ephemeralCounters.emplace_back(std::move(counter));
        }
        else {
          _currentTxn->add(counter);
        }
      }
    } else {
      if(probe->canBeginTxn() || probe->canResumeTxn()) {
        _currentTxn = makeTxn(std::move(counter));
        if(!_ephemeralCounters.empty()) {
          _nonTxnSampleCount += _ephemeralCounters.size();
          _ephemeralCounters.clear();
        }
      } else if(probe->canEndTxn() || probe->canSuspendTxn()) {
        ++_compromisedTxnCount;
        _ephemeralCounters.clear();
      } else {
        _ephemeralCounters.emplace_back(std::move(counter));
      }
    }
    return ReturnCode::SUCCESS;
  }

}}
