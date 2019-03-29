///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite options for framework initialization and control of profile session
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <initializer_list>
#include <string>
#include <sstream>

namespace xpedite { namespace framework {

  enum Option {
    AWAIT_PROFILE_BEGIN,
    DISABLE_REMOTE_PROFILING 
  };

  using Options = std::initializer_list<Option>;

  inline std::string toString(Option option_) noexcept {
    switch(option_)
    {
      case AWAIT_PROFILE_BEGIN:
        return "Await profile begin";
      case DISABLE_REMOTE_PROFILING:
        return "Disable remote profiling";
    }
    return "Unknown";
  }

  template<typename Options>
  inline std::string toString(Options options_) noexcept {
    std::ostringstream stream;
    stream << "[";
    for(auto option : options_) {
      stream << toString(option);
    }
    stream << "]";
    return stream.str();
  }

}}
