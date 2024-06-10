////////////////////////////////////////////////////////////////////////////////////////////
//
// ProbeKey - A key to identify probes using name, location in source files.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>
#include <cstdint>

namespace xpedite { namespace probes {

  class ProbeKey
  {
    std::string _name;
    std::string _file;
    uint32_t _line;

    public:

    template<typename N>
    explicit ProbeKey(N&& name_)
      : _name {std::forward<N>(name_)}, _file {}, _line {} {
    }

    template<typename F, typename N>
    ProbeKey(N&& name_, F&& file_, uint32_t line_)
      : _name {std::forward<N>(name_)}, _file {std::forward<F>(file_)}, _line {line_} {
    }

    const std::string name()  const noexcept { return _name; }
    const std::string& file() const noexcept { return _file; }
    uint32_t line()           const noexcept { return _line; }
  };

  inline bool operator<(const ProbeKey& lhs_, const ProbeKey& rhs_) noexcept {
    if(lhs_.file() == rhs_.file()) {
      return lhs_.line() == rhs_.line() ? lhs_.name() < rhs_.name() : lhs_.line() < rhs_.line();
    }
    return lhs_.file() < rhs_.file();
  }

}}
