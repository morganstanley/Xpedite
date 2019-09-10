/*!
 * \file
 * Call Info struct.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include <string>


namespace vivify { namespace util {

/*!
 * \brief Call info.
 */
struct CallInfo
{
  struct Info
  {
    bool         _valid{false};   ///< The position is valid.
    std::string  _func{};         ///< Name of the function.
    std::string  _file{};         ///< Name of the source file.
    unsigned int _line{0u};       ///< Line number within the source file.
  };

  Info _info{};                   ///< Call info.
  Info _inlInfo{};                ///< Call inline info.
};

}} // namespace vivify::util
