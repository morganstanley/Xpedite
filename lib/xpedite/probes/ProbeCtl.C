///////////////////////////////////////////////////////////////////////////////
//
// Probes - Probes with near zero overhead, that can be activated at runtime
//
// Provides a collection of methods to
//   1. Lazy initialize thread sample buffers
//   2. Logic to locate, enable and disable probes
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/probes/Config.H>
#include <xpedite/probes/ProbeCtl.H>
#include <xpedite/probes/ProbeList.H>
#include <xpedite/util/Util.H>
#include <xpedite/util/AddressSpace.H>
#include <set>

void* xpediteTrampolinePtr {reinterpret_cast<void*>(xpediteTrampoline)};
void* xpediteDataProbeTrampolinePtr {reinterpret_cast<void*>(xpediteDataProbeTrampoline)};
void* xpediteIdentityTrampolinePtr {reinterpret_cast<void*>(xpediteIdentityTrampoline)};

namespace xpedite { namespace probes {

  void activateProbe(Probe* probe_) {
    if(!probe_->canActivate()) {
      return;
    }
    util::AddressSpace& asp (util::addressSpace());
    auto codeSegment = asp.find(probe_->rawCallSite());
    if(!codeSegment) {
      XpediteLogCritical << "failed to activate probe - cannot locate segments - code(" << codeSegment << ")" << XpediteLogEnd;
      return;
    }

    if(codeSegment->makeWritable()) {
      probe_->activate();
      codeSegment->restoreProtections();
    }
  }

  void deactivateProbe(Probe* probe_) {
    if(probe_->rawCallSite() == 0) {
      return;
    }

    util::AddressSpace& asp (util::addressSpace());
    if(auto codeSegment = asp.find(probe_->rawCallSite())) {
      if(codeSegment->makeWritable()) {
        probe_->deactivate();
        codeSegment->restoreProtections();
      }
    }
  }

  void probeCtl(Command cmd_, const char* file_, int line_, const char *name_) {
    util::AddressSpace& asp (util::addressSpace());
    std::set<util::AddressSpace::Segment*> segments;

    switch (cmd_) {
    case Command::ENABLE:
    case Command::DISABLE:

      for(auto& probe : probeList()) {
        if(probe.match(file_, line_, name_)) {
          segments.emplace(asp.find(probe.rawCallSite()));
        }
      }

      for(auto* segment : segments) {
        if(segment)
          segment->makeWritable();
      }

      for(auto& probe : probeList()) {
        if(probe.match(file_, line_, name_)) {
          if(config().verbose())
            log::logProbe(probe, (cmd_ == Command::ENABLE) ? "Probe Enable" : "Probe Disable");
          if(cmd_ == Command::ENABLE)
            probe.activate();
          else
            probe.deactivate();
        }
      }

      for(auto segment : segments) {
        if(segment)
          segment->restoreProtections();
      }
      break;
    case Command::REPORT:
      for(auto& probe : probeList()) {
        if(probe.match(file_, line_, name_)) {
          log::logProbe(probe, "Probe ");
        }
      }
      break;
    default:
      XpediteLogError << "probeCtl unknown cmd \" " << static_cast<int>(cmd_) << "\"" << XpediteLogEnd;
      break;
    }
  }
}}

