///////////////////////////////////////////////////////////////////////////////
//
// RequestParser - parses string to build request types
//
// Supports the following types of requests
//
// Ping               - Heartbeats to keep the external profiling session alive
// TscHz              - Request to estimate tscHz of the cpu
// ListProbes         - Request to list probes and their status in csv format
// ActivateProbe      - Request to activate a probe
//                        arguments (--file <filename> --line <line-no>, --name <name of the probe)
// DeactivateProbe    - Request to deactivates an active probe
//                        arguments (--file <filename> --line <line-no>, --name <name of the probe)
// ActivatePmu        - Request to activate general purpose and fixed PMU counters
//                        arguments (
//                          --gpCtrCount <number of general purpose counters> 
//                          --fixedCtrList <list of fixed counters>
//                        )
// ActivatePerfEvents - Request to activate PMU counters using perf events api
//                        arguments (--data <marshalled PMUCtlRequest object>)
//
// BeginProfile       - Request to activate a profiling session to collect tsc and counters
//                        arguments (
//                          --pollInterval <Interval to poll for samples>
//                          --samplesFilePattern <Wildcard for samples data files>
//                          --samplesDataCapacity <Max size of samples collected>
//                        )
// 
// EndProfile         - Request to deactivate profiling session
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Request.H"
#include <string>
#include <vector>
#include <memory>

namespace xpedite { namespace framework { namespace request {

  class RequestParser
  {
    RequestPtr parseArgs(const std::string& cmd_, const std::vector<const char*>& args_);

    public:

    RequestPtr parse(const char* data_, size_t len_);
  };

}}}
