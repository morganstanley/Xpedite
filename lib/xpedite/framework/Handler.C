////////////////////////////////////////////////////////////////////////////////////////
//
// Handler to lookup and execute commands from profiler
//
// Handler provides the following functionality.
//   1. Api to register commands and callbacks
//   2. Tokenizer to extract command and arguments from frames
//   3. Command mapping and execution of callbacks
//   4. Support for heartbeats, starting and stopping of profiling sessions
// 
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////

#include "Handler.H"
#include <xpedite/util/Tsc.H>
#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/log/Log.H>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <thread>
#include <string>

namespace xpedite { namespace framework {

  std::string ping(Profile&, const std::vector<const char*>&) {
    return "hello";
  }

  std::string tscHz(Profile&, const std::vector<const char*>&) {
    auto tscHz = util::estimateTscHz();
    return std::to_string(tscHz);
  }

  std::string Handler::beginProfile(Profile& profile_, const std::vector<const char*>& args_) {
    if(args_.size() < 2) {
      std::ostringstream stream;
      stream << "xpedite - failed to begin profile - command \"BeginProfile\" missing arguments expected 2, got only " << args_.size();
      auto errMsg = stream.str();
      XpediteLogError << errMsg << XpediteLogEnd;
      return errMsg;
    }

    _pollInterval = std::chrono::duration<unsigned, std::milli> {std::stoi(args_[1])};
    XpediteLogInfo << "xpedite - starting collecter sample file - " << args_[0]
       << " | poll interval - every " << _pollInterval.count() << " milli seconds." << XpediteLogEnd;
    _collector.reset(new Collector {args_[0]});

    if(!_collector->beginSamplesCollection()) {
      std::ostringstream stream;
      stream << "xpedite - failed to initialize collector - check application stdout for more details";
      auto errMsg = stream.str();
      XpediteLogError << errMsg << XpediteLogEnd;
      _collector.reset();
      return errMsg;
    }
    profile_.start();
    return {};
  }

  std::string Handler::endProfile(Profile& profile_, const std::vector<const char*>&) {
    if(!_collector) {
      return "profiling not active - can't end something that's not started";
    }

    _collector->endSamplesCollection();
    _collector.reset();
    profile_.stop();
    return {};
  }

  Handler::Handler()
    : _cmdMap {
        {"ping", ping}
       ,{"tscHz", tscHz}
       ,{"beginProfile", [this](Profile& profile_, const std::vector<const char*>& args_){return beginProfile(profile_, args_);}}
       ,{"endProfile", [this](Profile& profile_, const std::vector<const char*>& args_){return endProfile(profile_, args_);}}
      }
    , _pollInterval {10} /*10 milli second*/ {
  }

  void Handler::shutdown() {
    _collector.reset();
  }

  void Handler::poll() {
    if(_collector) {
      _collector->poll();
    }
    pmu::pmuCtl().poll();
    std::this_thread::sleep_for(_pollInterval);
  }

  bool Handler::registerCommand(std::string cmdName_, CmdProcessor processor_) {
    return _cmdMap.emplace(cmdName_, processor_).second;
  }

  std::string Handler::handle(const char* data_, size_t len_) {
    std::string argStr {data_, len_};
    XpediteLogInfo << "xpedite - handle command |" << argStr << "|" << XpediteLogEnd;
    const char* delimiter = " ";
    char *ptr;
    char *token = strtok_r(const_cast<char*>(argStr.c_str()), delimiter, &ptr);
    std::vector<const char*> args;
    if(token) {
      std::string cmd {token};
      auto iter = _cmdMap.find(cmd);
      if(iter == _cmdMap.end()) {
        return "unknown Command: " + cmd;
      }

      while((token = strtok_r(nullptr, delimiter, &ptr))) {
        args.emplace_back(token);
      }
      return (iter->second)(_profile, args);
    }
    return {};
  }

}}
