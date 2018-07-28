///////////////////////////////////////////////////////////////////////////////
//
// Utility method for probe administration
//
// The method provides the following commands to query and update status of
// probes in the application
//
// show    - returns a list of probes and their status in csv format
// enable  - activate a probe
//           optional arguments (--file <filename:line-no>, --name <name of the probe)
// disable - deactivates an active probe
//           optional arguments (--file <filename:line-no>, --name <name of the probe)
// log     - logs probe status to console
// pmu     - configures the number of type of pmc counters to be collected
//           arguments (--gpCtrCount <number of general purpose events>, 
//            -fixedCtrList <bitmap of fixed pmc events>)
// 
// The probes can  enable and disable using one of the following keys
//   1. Name of the probe
//   2. Location of the probe (filename and line number)
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include "Admin.H"
#include <xpedite/probes/ProbeCtl.H>
#include <xpedite/pmu/EventSelect.h>
#include <xpedite/probes/ProbeList.H>
#include <xpedite/util/Util.H>
#include <xpedite/log/Log.H>
#include "../framework/Profile.H"
#include <cstring>
#include <sstream>

namespace xpedite { namespace framework {

  namespace {
    const std::string CMD_SHOW        { "show"    };
    const std::string CMD_ENABLE      { "enable"  };
    const std::string CMD_DISABLE     { "disable" };
    const std::string CMD_PMU         { "pmu"     };

    const std::string OPT_FILE        { "--file"         };
    const std::string OPT_LINE        { "--line"         };
    const std::string OPT_NAME        { "--name"         };
    const std::string OPT_PMU_COUNT   { "--gpCtrCount"   };
    const std::string OPT_PMU_FIXED   { "--fixedCtrList" };
    const std::string OPT_PMU_REQUEST { "--request"      };
  }

  template<typename Extractor>
  void extractArguments(Extractor extractor_, const std::vector<const char*>& args_) {
    for (unsigned i=1; i+1 < args_.size(); i+=2) {
      extractor_(args_[i], args_[i+1]);
    }
  }

  static std::string parseRequest(const std::string& reqStr_, PMUCtlRequest& request_) noexcept {
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

  std::string admin(Profile& profile_, const std::vector<const char*>& args_) {
    std::string retVal = "";
    if(args_.size() == 0 || (args_.size() == 1 && (args_[0] == CMD_SHOW))) {
      std::ostringstream stream;
      log::logProbes(stream, probes::probeList());
      retVal = stream.str();
    }
    else if(args_.size() > 0 && (args_[0] == CMD_ENABLE || args_[0] == CMD_DISABLE)) {
      std::string file = "";
      std::string name = "";
      uint32_t line {};
      extractArguments([&](const char* name_, const char* value_) {
        if     (name_ == OPT_FILE) { file = value_;       }
        else if(name_ == OPT_LINE) { line = atoi(value_); }
        else if(name_ == OPT_NAME) { name = value_;       }
      }, args_);
      if(args_[0] == CMD_ENABLE) {
        profile_.enableProbe(name, file, line);
      }
      else {
        profile_.disableProbe(name, file, line);
      }
    }
    else if(args_.size() > 0 && args_[0] == CMD_PMU) {
      extractArguments([&](const char* name_, const char* value_) {
        if(name_ == OPT_PMU_COUNT) {
          profile_.enableGpPMC(atoi(value_));
        }
        else if(name_ == OPT_PMU_FIXED) {
          char opt[strlen(value_)+1];
          strcpy(opt, value_);
          char* ptr;
          const char* delimiter {","};
          char* token = strtok_r(opt, delimiter, &ptr);
          while(token) {
            profile_.enableFixedPMC(atoi(token));
            token= strtok_r(nullptr, delimiter, &ptr);
          }
        }
        else if(name_ == OPT_PMU_REQUEST) {
          PMUCtlRequest request;
          retVal = parseRequest(value_, request);
          if(retVal.empty()) {
            if(!profile_.enablePMC(request)) {
              retVal = "failed to enable pmu events in request";
            }
          }
          else {
            std::cout << retVal << std::endl;
          }
        }
      }, args_);
    }
    else {
      retVal = std::string{"Unknown Command: "} + args_[0];
    }
    return retVal;
  }

}}
