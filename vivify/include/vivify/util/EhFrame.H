/*!
 * \file
 * .eh_frame representation.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include <cstdint>

namespace vivify { namespace util {

/*!
 * \brief .eh_frame representation.
 */
struct EhFrame
{
  uint64_t _offset{0u};
  uint64_t _fdeCount{0u};
  uint64_t _tableData{0u};

  auto segbase() const noexcept { return _offset; }
};

}} // namespace vivify::util
