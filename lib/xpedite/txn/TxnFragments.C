///////////////////////////////////////////////////////////////////////////////
//
// Logic to build transactions from multiple fragments
// 
// This module handles loading of transaction fragments from multiple threads.
// 
// The loaded fragments are linked (suspending to resuming and vice versa) to
// create a chain of framgents to complete a transaction.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/txn/TxnFragments.H>

namespace xpedite { namespace txn {


  void TxnFragments::joinFragments(TxnId& nextTxnId_, Txns& txns_, TxnPtr txn_, ResumeFragments* fragments_) {
    if(!fragments_) {
      auto txnId {++nextTxnId_};
      txn_->setId(txnId);
      txns_.emplace(txnId, std::move(txn_));
    } else if(fragments_->data().size() == 1) {
      auto& rfrag = fragments_->data()[0];
      txn_->join(rfrag._txn);
      joinFragments(nextTxnId_, txns_, std::move(txn_), rfrag.resumeFragments());
    }
    else {
      auto txnClone = *txn_;
      auto rfagCount = fragments_->data().size();
      for(size_t i=0; i<rfagCount; ++i) {
        auto& rfrag = fragments_->data()[i]; 
        txn_->join(rfrag._txn);
        joinFragments(nextTxnId_, txns_, std::move(txn_), rfrag.resumeFragments());
        if(i < rfagCount -1) {
          txn_ = std::make_shared<Txn>(txnClone);
        } else {
          txn_ = std::make_shared<Txn>(txnClone);
        }
      }
    }
  }

  /*
   * Adds a resuming transaction fragment to the collection.
   * Resuming fragments lookup for the previous suspended fragement to form a link
  */
  void TxnFragments::addTxn(TxnPtr txn_, std::optional<LinkId> resumeId_, std::optional<LinkId> suspendId_) {
    if(resumeId_) {
      ResumeFragment resumeFragment {*resumeId_, std::move(txn_)};
      if(suspendId_) {
        resumeFragment.setResumeFragments(&getResumeFragments(*suspendId_));
      }
      getResumeFragments(*resumeId_).addFragment(std::move(resumeFragment));
    } else {
      assert(suspendId_);
      _rootFragments.emplace_back(Fragment {std::move(txn_)});
    }
  }


  Txns TxnFragments::join(TxnId nextTxnId_) {
    Txns txns;
    for(auto& fragment : _rootFragments) {
      joinFragments(nextTxnId_, txns, std::move(fragment._txn), fragment.resumeFragments());
    }
    return txns;
  }


}}
