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

#pragma once
#include <xpedite/txn/Txn.H>
#include <xpedite/txn/Counter.H>
#include <vector>
#include <memory>
#include <optional>
#include <sstream>

namespace xpedite { namespace txn {

  class LinkId {

    uint64_t _tlsAddr;
    uint64_t _tsc;

    public:


    LinkId()
      : _tlsAddr {}, _tsc {} {
    };

    LinkId(uint64_t tlsAddr_, uint64_t tsc_)
      : _tlsAddr {tlsAddr_}, _tsc {tsc_} {
    }

    uint64_t tsc()      const noexcept { return _tsc;     }
    uint64_t tlsAddr()  const noexcept { return _tlsAddr; }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Link id {tlsAddr : " << _tlsAddr << ", tsc : " << _tsc << "}";
      return stream.str();
    }
  };

  struct LinkIdHash {
    size_t operator()(const LinkId& id_) const {
      return std::hash<uint64_t>{}(id_.tlsAddr()) ^ std::hash<uint64_t>{}(id_.tsc());
    }
  };

  inline bool operator==(const LinkId& lhs_, const LinkId& rhs_) {
    return lhs_.tlsAddr() == rhs_.tlsAddr() && lhs_.tsc() == rhs_.tsc();
  }

  class ResumeFragments;
  class TxnFragments;

  class Fragment {

    friend class TxnFragments;

    TxnPtr _txn;
    ResumeFragments* _resumeFragments;

    public:

    explicit Fragment(TxnPtr txn_)
      : _txn {std::move(txn_)}, _resumeFragments {} {
    }


    ResumeFragments* resumeFragments() const noexcept { return _resumeFragments; }
    void setResumeFragments(ResumeFragments* resumeFragments_) noexcept {
      _resumeFragments = resumeFragments_;
    }

    const Txn& tnx() const noexcept {
      return *_txn;
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Fragment Txn id - " << _txn->id() << " | data - " << _txn->toString();
      return stream.str();
    }
  };

  class ResumeFragment : public Fragment {

    LinkId _id;

    public:

    ResumeFragment(LinkId id_, TxnPtr txn_)
      : Fragment {std::move(txn_)}, _id {id_} {
    }

    LinkId id() const noexcept { return _id; }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Resume fragment - " << id().toString() << " | " << Fragment::toString();
      return stream.str();
    }
  };

  class ResumeFragments {
    LinkId _id;
    std::vector<ResumeFragment> _data;

    public:

    explicit ResumeFragments(LinkId id_)
      : _id {id_}, _data {} {
    }

    LinkId id() const noexcept                   { return _id;   }
    std::vector<ResumeFragment>& data() noexcept { return _data; }

    void addFragment(ResumeFragment fragment_) {
      assert(_id == fragment_.id());
      _data.emplace_back(std::move(fragment_));
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "ResumeFragments  Link id - " << _id.toString() << " | count - " << _data.size() 
        << std::endl;
      for(const auto& rfrag : _data) {
        stream << rfrag.toString() << std::endl;
      }
      return stream.str();
    }
  };

  class TxnFragments
  {
    std::unordered_map<LinkId, ResumeFragments, LinkIdHash> _resumptions;
    std::vector<Fragment> _rootFragments;
    TxnId _nextTxnId;

    void joinFragments(TxnId& nextTxnId_, Txns& txns_, TxnPtr txn_, ResumeFragments* fragments_);

    ResumeFragments& getResumeFragments(LinkId id_) {
      if(auto it = _resumptions.find(id_); it != _resumptions.end()) {
        return it->second;
      }
      auto it = _resumptions.emplace(id_, ResumeFragments {id_});
      assert(it.second);
      return it.first->second;
    }

    public:

    void addTxn(TxnPtr txn_, std::optional<LinkId> resumeId_, std::optional<LinkId> suspendId_);
    Txns join(TxnId nextTxnId_);
  };

}}
