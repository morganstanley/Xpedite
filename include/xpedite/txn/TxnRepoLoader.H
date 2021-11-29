///////////////////////////////////////////////////////////////////////////////
//
// Transaction repository factory
// 
// Factory to build a repository of transactions
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/ux/UxProbe.H>
#include <xpedite/txn/TxnRepo.H>
#include <xpedite/txn/TxnLoader.H>
#include <xpedite/txn/DataSource.H>
#include <xpedite/log/Log.H>
#include <string>
#include <map>

namespace xpedite::txn {

  class TxnRepoLoader
  {
    TxnRepoPtr _repo;

    public:

    TxnRepoLoader()
      : _repo {} {
    }

    //Builds a repository of transactions for current profile session and benchmarks
    bool load(DataSource dataSource_, std::vector<ux::UxProbe> uxProbes_);

    const TxnRepoPtr data() {
      return _repo;
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "TxnRepo loader";
      return stream.str();
    }
  };
}
