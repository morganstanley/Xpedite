//////////////////////////////////////////////////////////////////////////////////////////////
//
// Request - request to execute an action for a profile session
// 
// Requests are hierarchical group of classes, that on execution alter or setup
// parameters for a profiling session.
//
// Each request, holds data for command(s) to be executed and response of execution.
// Upon execution, the response is populated with a result or errors in case of failure.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "AbstractRequest.H"
#include "../Handler.H"
#include <string>
#include <sstream>

namespace xpedite { namespace framework { namespace request {

  class Response : public AbstractResponse
  {
    std::string _value;

    public:

    void setValue(std::string value_) {
      _value = std::move(value_);
      _status = Status::SUCCESS;
    }

    const std::string& value() const override {
      return _value;
    }
  };

  class Request : public AbstractRequest {

    protected:

    Response _response;

    static std::string toString(Status status_);

    std::string toString(const char* type_) const;

    virtual const char* typeName() const {
      return "Request";
    }

    public:

    const Response& response() const override {
      return _response;
    }

    Response& response() override {
      return _response;
    }

    std::string toString() const override {
      return toString(typeName());
    }
  };

  struct PingRequest : public Request {

    void execute(Handler& handler_) override {
      _response.setValue(handler_.ping());
    }

    const char* typeName() const override {
      return "PingRequest";
    }

  };

  struct TscRequest : public Request {

    void execute(Handler& handler_) override {
      auto tscHz = handler_.tscHz();
      _response.setValue(std::to_string(tscHz));
    }

    const char* typeName() const override {
      return "TscRequest";
    }

  };

  struct InvalidRequest : public Request {

    std::string _errors;

    public:

    explicit InvalidRequest(std::string errors_)
      : _errors {std::move(errors_)} {
    }

    void execute(Handler&) override {
      _response.setErrors(_errors);
    }

    const char* typeName() const override {
      return "InvalidRequest";
    }
  };

}}}
