///////////////////////////////////////////////////////////////////////////////
//
// Transaction repository factory
// 
// Factory to build a repository of transactions
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/txn/TxnRepoLoader.H>
#include <xpedite/persistence/SamplesLoader.H>

namespace xpedite::txn {

  bool TxnRepoLoader::load(DataSource dataSource_, std::vector<ux::UxProbe> uxProbes_) {
    if(dataSource_.files().empty()) {
      return {};
    }
    using SamplesLoader = persistence::SamplesLoader;
    std::vector<SamplesLoader> samplesLoaders;
    std::optional<TxnLoader> loader;
    std::optional<persistence::ProfileInfo> profileInfo;
    for(const auto& file : dataSource_.files()) {
      XpediteLogInfo << "loading transactions from - " << file << XpediteLogEnd;
      auto& samplesLoader {samplesLoaders.emplace_back(SamplesLoader {file.path().c_str()})};
      if(!loader) {
        profileInfo = samplesLoader.loadProfileInfo(std::move(uxProbes_));
        loader = TxnLoader {profileInfo->probes()};
        loader->beginCollection();
      }
      loader->beginLoad(file.threadId(), file.tlsAddr());
      for(auto& sample : samplesLoader) {
        loader->load(&sample);
      }
      loader->endLoad();
      XpediteLogInfo << "loaded " << loader->txnCount() << " transactions." << XpediteLogEnd;
    }
    loader->endCollection();
    if(loader->txnCount() <=0) {
      if(loader->sampleCount()) {
        XpediteLogInfo << "failed to load transactions."
          << " Recheck routes specified in your profile info" << XpediteLogEnd;
        return {};
      }
      XpediteLogInfo << "failed to load transactions."
        << " It appears the app hit any of the activated probes" << XpediteLogEnd;
        return {};
    }
    _repo = std::make_shared<TxnRepo>();
    auto collection = std::make_shared<TxnCollection>(
      dataSource_.name(), std::move(*profileInfo), std::move(samplesLoaders), loader->moveTxns()
    );
    _repo->setCurrent(std::move(collection));
    return true;
  }

}
