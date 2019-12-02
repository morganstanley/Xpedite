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
#include "../Handler.H"
#include <string>
#include <sstream>

namespace xpedite { namespace framework { namespace request {

  enum class Status {
    SUCCESS,
    NOT_READY,
    FAILED
  };

  struct AbstractResponse
  {
    AbstractResponse()
      : _status {Status::NOT_READY}, _errors {} {
    }

    virtual ~AbstractResponse() = default;

    Status status() const noexcept {
      return _status;
    }

    const std::string& errors() const noexcept {
      return _errors;
    }

    void setErrors(std::string errors_) {
      _errors = std::move(errors_);
      _status = Status::FAILED;
    }

    explicit operator bool() const noexcept {
      return _status == Status::SUCCESS;
    }

    virtual const std::string& value() const = 0;

    protected:

    Status _status;
    std::string _errors;
  };


  struct AbstractRequest
  {
    using Handler = framework::Handler;

    void abort(std::string errors_) {
      response().setErrors(std::move(errors_));
    }

    virtual ~AbstractRequest() = default;
    virtual const AbstractResponse& response() const = 0;
    virtual AbstractResponse& response() = 0;
    virtual void execute(Handler& handler_) = 0;
    virtual std::string toString() const = 0;
  };

  using RequestPtr = std::unique_ptr<AbstractRequest>;

}}}
