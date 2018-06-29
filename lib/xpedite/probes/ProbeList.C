///////////////////////////////////////////////////////////////////////////////
//
// ProbeList - A collection of iterable probes
//
// Provides logic to 
//  1. build a linked list of probes during process initialization
//  2. linked list cleanup and probe removal during process shutdown
//
//  The state of probes are validated at the time of addition and removal
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/probes/Config.H>
#include <xpedite/probes/ProbeList.H>
#include <xpedite/probes/ProbeCtl.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <cstdio>

xpedite::probes::ProbeList* xpedite::probes::ProbeList::_instance;

extern "C" {

  void XPEDITE_CALLBACK xpediteAddProbe(xpedite::probes::Probe* probe_, xpedite::probes::CallSite callSite_, xpedite::probes::CallSite returnSite_) {
    using namespace xpedite::probes;

    if(XPEDITE_UNLIKELY(!probe_)) {
      fprintf(stderr, "failed to add probe - addProbe invoked with nullptr\n");
      return;
    }

    auto isValid = probe_->isValid(callSite_, returnSite_);
    if(XPEDITE_UNLIKELY(config().verbose())) {
      auto probeStr = probe_->toString();
      fprintf(stderr, "adding probe %s | status - %s\n", probeStr.c_str(), callSite_ &&  isValid ? "Valid" : "InValid");
    }

    if(!isValid) {
      return;
    }

    if(XPEDITE_UNLIKELY(probe_->_next)) {
      auto probeStr = probe_->toString();
      fprintf(stderr, "failed to add probe %s - detected double initialization for probe\n", probeStr.c_str());
      return;
    }

    ProbeList::get().add(probe_);
  }
 
  void XPEDITE_CALLBACK xpediteRemoveProbe(xpedite::probes::Probe* probe_) {
    using namespace xpedite::probes;
    if(config().verbose()) {
      auto probeStr = probe_->toString();
      fprintf(stderr, "removing probe %s\n", probeStr.c_str());
    }
    ProbeList::get().remove(probe_);
  }
}
