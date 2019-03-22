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

#include "RequestParser.H"
#include "ProbeRequest.H"
#include "ProfileRequest.H"
#include <xpedite/probes/ProbeKey.H>
#include <xpedite/pmu/EventSet.h>
#include <xpedite/util/Util.H>
#include <xpedite/log/Log.H>
#include <cstring>
#include <string>

namespace xpedite { namespace framework { namespace request {

  namespace {

    const std::string REQ_PING                          { "Ping"                 };
    const std::string REQ_TSC_HZ                        { "TscHz"                };
    const std::string REQ_PROBE_LIST                    { "ListProbes"           };

    const std::string REQ_PROBE_ACTIVATION              { "ActivateProbe"        };
    const std::string REQ_PROBE_DEACTIVATION            { "DeactivateProbe"      };
    const std::string ARG_FILE                          { "--file"               };
    const std::string ARG_LINE                          { "--line"               };
    const std::string ARG_NAME                          { "--name"               };

    const std::string REQ_PMU_ACTIVATION                { "ActivatePmu"          };
    const std::string ARG_PMU_COUNT                     { "--gpCtrCount"         };
    const std::string ARG_PMU_FIXED                     { "--fixedCtrList"       };

    const std::string REQ_PERF_EVENTS_ACTIVATION        { "ActivatePerfEvents"   };
    const std::string ARG_PERF_EVENTS_DATA              { "--data"               };

    const std::string REQ_PROFILE_ACTIVATION            { "BeginProfile"         };
    const std::string ARG_PROFILE_POLL_INTERVAL         { "--pollInterval"       };
    const std::string ARG_PROFILE_SAMPLES_FILE_PATTERN  { "--samplesFilePattern" };
    const std::string ARG_PROFILE_SAMPLES_DATA_CAPACITY { "--samplesDataCapacity" };

    const std::string REQ_PROFILE_DEACTIVATION          { "EndProfile"           };
  }

  template<typename Extractor>
  void extractArguments(Extractor extractor_, const std::vector<const char*>& args_) {
    for (unsigned i=0; i+1 < args_.size(); i+=2) {
      extractor_(args_[i], args_[i+1]);
    }
  }

  static std::string parsePmuRequest(const std::string& reqStr_, PMUCtlRequest& request_) noexcept {
    auto expectedSize = 3 * sizeof(PMUCtlRequest) - 1;
    if(reqStr_.size() != expectedSize) {
      std::ostringstream stream;
      stream << "Detected invalid pmu request - expected  " << expectedSize << " bytes "
        << "recieved " << reqStr_.size() << "bytes";
      return stream.str();
    }
    uint8_t* ptr {reinterpret_cast<uint8_t*>(&request_)};
    const char* buffer {reqStr_.c_str()};
    bool isValid {true};
    for(unsigned i=0; i<reqStr_.size() && isValid; i+=3) {
      std::tie(*ptr++, isValid) = util::atoiHex(buffer + i);
      if(!isValid) {
        std::ostringstream stream;
        stream << "Detected invalid number at offset " << i;
        return stream.str();
      }
    }
    return {};
  }

  RequestPtr RequestParser::parse(const char* data_, size_t len_) {
    std::string argStr {data_, len_};
    XpediteLogInfo << "xpedite - parsing request |" << argStr << "|" << XpediteLogEnd;
    const char* delimiter = " ";
    char *ptr;
    char *token = strtok_r(const_cast<char*>(argStr.c_str()), delimiter, &ptr);
    std::vector<const char*> args;
    if(token) {
      std::string req {token};
      while((token = strtok_r(nullptr, delimiter, &ptr))) {
        args.emplace_back(token);
      }
      return parseArgs(req, args);
    }
    return RequestPtr {new InvalidRequest {"Empty request ..."}};
  }

  RequestPtr RequestParser::parseArgs(const std::string& req_, const std::vector<const char*>& args_) {
    std::string errors;
    if(req_ == REQ_PING) {
      return RequestPtr {new PingRequest {}};
    }
    else if(req_ == REQ_TSC_HZ) {
      return RequestPtr {new TscRequest {}};
    }
    else if(req_ == REQ_PROBE_LIST) {
      return RequestPtr {new ProbeListRequest {}};
    }
    else if(args_.size() > 0 && (req_ == REQ_PROBE_ACTIVATION || req_ == REQ_PROBE_DEACTIVATION)) {
      std::string file = "";
      std::string name = "";
      uint32_t line {};
      extractArguments([&](const char* name_, const char* value_) {
        if     (name_ == ARG_FILE) { file = value_;       }
        else if(name_ == ARG_LINE) { line = atoi(value_); }
        else if(name_ == ARG_NAME) { name = value_;       }
      }, args_);
      probes::ProbeKey key {name, file, line};
      if(req_ == REQ_PROBE_ACTIVATION) {
        return RequestPtr {new ProbeActivationRequest {{key}}};
      }
      else {
        return RequestPtr {new ProbeDeactivationRequest {{key}}};
      }
    }
    else if(args_.size() > 0 && req_ == REQ_PMU_ACTIVATION) {
      int gpEventsCount {};
      std::vector<int> fixedEventIndices;
      extractArguments([&](const char* name_, const char* value_) {
        if(name_ == ARG_PMU_COUNT) {
          gpEventsCount = atoi(value_);
        }
        else if(name_ == ARG_PMU_FIXED) {
          char opt[strlen(value_)+1];
          strcpy(opt, value_);
          char* ptr;
          const char* delimiter {","};
          char* token = strtok_r(opt, delimiter, &ptr);
          while(token) {
            fixedEventIndices.emplace_back(atoi(token));
            token= strtok_r(nullptr, delimiter, &ptr);
          }
        }
      }, args_);
      return RequestPtr {new PmuActivationRequest {gpEventsCount, fixedEventIndices}};
    }
    else if(args_.size() > 0 && req_ == REQ_PERF_EVENTS_ACTIVATION) {
      PMUCtlRequest request {};
      extractArguments([&](const char* name_, const char* value_) {
        if(name_ == ARG_PERF_EVENTS_DATA) {
          errors = parsePmuRequest(value_, request);
        }
      }, args_);
      if(errors.empty()) {
        return RequestPtr {new PerfEventsActivationRequest {request}};
      }
    }
    else if(args_.size() > 0 && req_ == REQ_PROFILE_ACTIVATION) {
      std::string samplesFilePattern;
      MilliSeconds pollInterval {};
      uint64_t samplesDataCapacity {};
      extractArguments([&](const char* name_, const char* value_) {
        if(name_ == ARG_PROFILE_SAMPLES_FILE_PATTERN) {
          samplesFilePattern = value_;
        }
        else if(name_ == ARG_PROFILE_POLL_INTERVAL) {
          pollInterval = MilliSeconds {std::stoi(value_)};
        }
        else if(name_ == ARG_PROFILE_SAMPLES_DATA_CAPACITY) {
          samplesDataCapacity = static_cast<uint64_t>(std::stol(value_));
        }
      }, args_);
      return RequestPtr {new ProfileActivationRequest {samplesFilePattern, pollInterval, samplesDataCapacity}};
    }
    else if(req_ == REQ_PROFILE_DEACTIVATION) {
      return RequestPtr {new ProfileDeactivationRequest {}};
    }
    else {
      errors = std::string{"Invalid Request: "} + req_;
    }
    return RequestPtr {new InvalidRequest {errors}};
  }

}}}
