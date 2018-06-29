///////////////////////////////////////////////////////////////////////////////
//
// Utility class to store probe configurations
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

namespace xpedite { namespace probes {

  class Config
  {
    bool _verbose;

    static Config* _instance;

    public:

    Config();

    bool verbose()    const noexcept { return _verbose;    }

    void setVerbose(bool value_) noexcept {
      _verbose = value_;
    }

    static Config& get() noexcept {
      if(!_instance) {
        _instance = new Config {};
      }
      return *_instance;
    }
  };

  inline Config& config() noexcept {
    return Config::get();
  }
}}
