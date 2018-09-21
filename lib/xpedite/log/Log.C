///////////////////////////////////////////////////////////////////////////////
//
// Global static definitions for logger
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <ios>
#include <iomanip>
#include <unistd.h>
#include <xpedite/log/Log.H>
#include <xpedite/probes/Probe.H>
#include <xpedite/probes/ProbeList.H>

namespace xpedite { namespace log {

  uint64_t logCounter = 0;

  void logProbe(std::ostream& logfile_, const probes::Probe& probe_, const char* action_) {
    if(action_) {
      logfile_ << "Action=" << action_ << " | ";
    }

    logfile_ << "Id=" 
      << std::setfill('0') << std::setw(4) << probe_.id() << std::setfill(' ') 
      << " | Probe=" << &probe_ 
      << " | CallSite=" << std::hex << reinterpret_cast<const void*>(probe_.rawCallSite()) << std::dec 
      << " | RecorderReturnSite=" << std::hex << reinterpret_cast<const void*>(probe_.recorderReturnSite()) << std::dec 
      << " | Status=" << (probe_.isActive() ? "enabled" : "disabled")
      << " | Name=" << probe_.name() 
      << " | File=" << probe_.file() 
      << " | Line=" << probe_.line() 
      << " | Function=" << probe_.func()
      << " | Attributes=" << probe_.attr().toString() << std::endl;
  }

  void logProbe(const probes::Probe& probe_, const char* action_) {
    logProbe(std::cout, probe_, action_);
  }

  void logProbes(std::ostream& logfile_, const probes::ProbeList& probeList_) {
    for(auto& probe : probeList_) {
      log::logProbe(logfile_, probe);
    }
  }

  void logProbes(const probes::ProbeList& probeList_) {
    logProbes(std::cout, probeList_);
  }
}}
