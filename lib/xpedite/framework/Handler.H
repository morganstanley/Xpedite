///////////////////////////////////////////////////////////////////////////////////////
//
// Handler to lookup and execute commands from profiler
// Supports registration, lookup and execution of commands for a profiling session.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include "Collector.H"
#include "Profile.H"

namespace xpedite { namespace framework {

  using CmdProcessor = std::function<std::string(Profile&, const std::vector<const char*>&)>;

  class Handler
  {
    public:

      Handler();

      std::string handle(const char* data_, size_t len);
      bool registerCommand(std::string cmdName_, CmdProcessor processor_);

      std::string beginProfile(Profile& profile_, const std::vector<const char*>& args_);
      std::string endProfile(Profile& profile_, const std::vector<const char*>& args_);

      void beginSession() {
      }

      void endSession() {
        endProfile(_profile, {});
      }

      bool isProfileActive() const noexcept {
        return static_cast<bool>(_collector);
      }

      void poll();
      void shutdown();

    private:

      std::map<std::string, CmdProcessor> _cmdMap;
      std::unique_ptr<Collector> _collector;
      std::chrono::duration<unsigned, std::milli> _pollInterval;
      Profile _profile;
  };

}}
