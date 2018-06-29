//////////////////////////////////////////////////////////////////////////////////////////////
//
// Utility method for probe administration
//
// The method provides the following commands, to query or update state of
// one or more probes in the application
//
// show    - returns a list of probes and their status in csv format
// enable  - activates a probe
//           optional arguments (--file <filename:line-no>, --name <name of the probe)
// disable - deactivates an active probe
//           optional arguments (--file <filename:line-no>, --name <name of the probe)
// log     - logs probe status to console
// pmu     - configures the number and type of pmc counters to be collected
//           required arguments (
//              --gpCtrCount <number of general purpose events>, 
//              --fixedCtrList <bitmap of fixed pmc events>)
// 
// The probes can be located for activation/deactivation, with one of the following keys
//   1. Name of the probe
//   2. Location of the probe (filename and line number)
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>
#include <vector>

namespace xpedite { namespace framework {

  class Profile;

  std::string admin(framework::Profile& profile_, const std::vector<const char*>& args_);

}}
