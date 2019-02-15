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

#include "Request.H"

namespace xpedite { namespace framework { namespace request {

  std::string Request::toString(Status status_) {
    switch(status_) {
      case Status::SUCCESS:
        return "Success";
      case Status::NOT_READY:
        return "Not Ready";
      case Status::FAILED:
        return "Failed";
    };
    return "Unknown";
  }

  std::string Request::toString(const char* type_) const {
    std::ostringstream stream;
    stream << type_ << "{ status - " << Request::toString(response().status());
    if(response().status() == Status::FAILED) {
      stream << " | errors - " << response().errors();
    } else if(response().status() == Status::SUCCESS) {
      stream << " | value - " << response().value();
    }
    stream << " }";
    return stream.str();
  }
}}}
