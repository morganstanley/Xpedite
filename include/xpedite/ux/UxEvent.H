///////////////////////////////////////////////////////////////////////////////
//
// UxProbe - Human friendly pmu event used in analytics and reporting
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <sstream>

namespace xpedite { namespace ux {

  class UxEvent {

    std::string _name;
    bool _user;
    bool _kernel;

    public:

    UxEvent()
      : _name {}, _user {}, _kernel {} {
    }

    explicit UxEvent(std::string name_)
      : _user {true}, _kernel {true} {
      if(auto p = name_.find(':'); p != std::string::npos) {
        if(p+1 < name_.size()) {
          auto flag = name_[p+1];
          _user = (flag == 'u' || flag == 'U');
          _kernel = (flag == 'k' || flag == 'K');
        }
        _name = std::string {std::move(name_), 0, p};
      } else {
        _name = std::move(name_);
      }
    }

    UxEvent(std::string name_, bool user_, bool kernel_)
      : _name {std::move(name_)}, _user {user_}, _kernel {kernel_} {
    }

    const std::string& name() const noexcept { return _name;    }
    bool user()               const noexcept { return _user;    }
    bool kernel()             const noexcept { return _kernel;  }

    std::string toString() const {
      std::ostringstream stream;
      stream << "Event [name : " << name() << ", user : " << (user() ? "true" : "false")
        << ", kernel : " << (kernel() ? "true" : "false") << "]";
      return stream.str();
    }
  };

}}
