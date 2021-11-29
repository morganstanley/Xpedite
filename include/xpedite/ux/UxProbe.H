///////////////////////////////////////////////////////////////////////////////
//
// UxProbe - Human friendly probes used in analytics and reporting
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>
#include <sstream>

namespace xpedite::ux {

  enum class ProbeType {
    Invalid = 0,
    TxnBeginProbe,
    TxnSuspendProbe,
    TxnResumeProbe,
    TxnEndProbe
  };

  inline std::string toString(ProbeType type_) {
    switch(type_) {
      case ProbeType::Invalid:
        return "Invalid";
      case ProbeType::TxnBeginProbe:
        return "TxnBeginProbe";
      case ProbeType::TxnSuspendProbe:
        return "TxnSuspendProbe";
      case ProbeType::TxnResumeProbe:
        return "TxnResumeProbe";
      case ProbeType::TxnEndProbe:
        return "TxnEndProbe";
    }
    return "Invalid";
  }

  class UxProbe {
    std::string _name;
    std::string _sysName;
    ProbeType _type;

    public:

    UxProbe()
      : _name {}, _sysName {}, _type {} {
    }

    UxProbe(std::string name_, std::string sysName_, ProbeType type_)
      : _name {std::move(name_)}, _sysName {std::move(sysName_)}, _type {type_} {
    }

    const std::string& name()    const noexcept { return _name;    }
    const std::string& sysName() const noexcept { return _sysName; }
    ProbeType type()             const noexcept { return _type;    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "UxProbe {name : " << name() << ", sysName : " << sysName()
        << ", type : " << ux::toString(type()) << "}";
      return stream.str();
    }
  };

}
