///////////////////////////////////////////////////////////////////////////////
//
// Utility class to lookup and store probe configurations
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/probes/Config.H>
#include <xpedite/probes/RecorderCtl.H>
#include <cstdlib>

namespace xpedite { namespace probes {

  const char* CONF_VERBOSE     {"XPEDITE_VERBOSE"};

  Config::Config()
    : _verbose {} {
    if(getenv(CONF_VERBOSE)) {
      _verbose = true;
    }
  }

  Config* Config::_instance;
}}
