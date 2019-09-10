/*!
 * \file
 * Stack Call Info struct.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include <cstdint>

#include "util/CallInfo.H"


namespace vivify {

struct StackCallInfo : public util::CallInfo
{
  uintptr_t   _ip{0u};  ///< Instruction pointer.
  std::string _bfile;   ///< Name of the binary file.
};

} // namespace vivify
