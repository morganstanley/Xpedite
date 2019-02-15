//////////////////////////////////////////////////////////////////////////////////////////////
//
// ProbeRequest - Group of request types to list, enable/disable probes
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Request.H"
#include <xpedite/probes/ProbeKey.H>

namespace xpedite { namespace framework { namespace request {

  struct ProbeListRequest : public Request {

    void execute(Handler& handler_) override {
      _response.setValue(handler_.listProbes());
    }

    const char* typeName() const override {
      return "ProbeListRequest";
    }
  };

  class ProbeActivationRequest : public Request {

    std::vector<probes::ProbeKey> _keys;

    public:
    
    ProbeActivationRequest(std::vector<probes::ProbeKey> keys_)
      : _keys {std::move(keys_)} {
    }

    void execute(Handler& handler_) override {
      for(const auto& key : _keys) {
        handler_.activateProbe(key);
      }
      _response.setValue("");
    }

    const char* typeName() const override {
      return "ProbeActivationRequest";
    }
  };

  class ProbeDeactivationRequest : public Request {

    std::vector<probes::ProbeKey> _keys;

    public:
    
    ProbeDeactivationRequest(std::vector<probes::ProbeKey> keys_)
      : _keys {std::move(keys_)} {
    }

    void execute(Handler& handler_) override {
      for(const auto& key : _keys) {
        handler_.deactivateProbe(key);
      }
      _response.setValue("");
    }

    const char* typeName() const override {
      return "ProbeDeactivationRequest";
    }
  };

}}}
