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

  using MilliSeconds = std::chrono::duration<unsigned, std::milli>;

  class Handler
  {
    public:

      Handler();

      std::string beginProfile(std::string samplesFilePattern_, MilliSeconds pollInterval_, uint64_t samplesDataCapacity_);
      std::string endProfile();

      bool isProfileActive() const noexcept {
        return static_cast<bool>(_collector);
      }

      std::string listProbes();
      void activateProbe(const probes::ProbeKey& key_);
      void deactivateProbe(const probes::ProbeKey& key_);

      void enableGpPMU(int count_);
      void enableFixedPMU(uint8_t index_);
      bool enablePerfEvents(const PMUCtlRequest& request_);
      void disablePMU();

      void poll();
      void shutdown();

      std::string ping() const noexcept;
      uint64_t tscHz() const noexcept;

      MilliSeconds pollInterval() const noexcept {
        return _pollInterval;
      }

    private:

      std::map<std::string, CmdProcessor> _cmdMap;
      std::unique_ptr<Collector> _collector;
      MilliSeconds _pollInterval;
      Profile _profile;
  };

}}
