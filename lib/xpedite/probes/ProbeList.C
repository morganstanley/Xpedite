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
      fprintf(stderr, "adding probe ['%s' at %s:%d] | status - %s\n",
        probe_->name(), probe_->file(), probe_->line(), probe_->callSite() &&  isValid ? "Valid" : "InValid");
    }

    if(!isValid) {
      return;
    }

    if(XPEDITE_UNLIKELY(probe_->_next)) {
      fprintf(stderr, "failed to add probe ['%s' at %s:%d] - detected double initialization for probe\n",
        probe_->name(), probe_->file(), probe_->line());
      return;
    }
    ProbeList::get().add(probe_);
  }
 
  void XPEDITE_CALLBACK xpediteRemoveProbe(xpedite::probes::Probe* probe_) {
    using namespace xpedite::probes;
    if(config().verbose()) {
      fprintf(stderr, "removing probe ['%s' at %s:%d]\n", probe_->name(), probe_->file(), probe_->line());
    }
    ProbeList::get().remove(probe_);
  }
}
