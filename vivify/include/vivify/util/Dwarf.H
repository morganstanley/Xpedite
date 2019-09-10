/*!
 * \file
 * DWARF utility class.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include <cstdint>

namespace vivify { namespace util {

/*!
 * \brief DWARF utility class.
 */
class Dwarf
{
public:
  /*!
   * \brief Constructs DWARF utility class.
   * \param ptr_ Pointer to a raw DWARF data.
   * \param end_ Pointer to the end of the data.
   */
  Dwarf(uint8_t* ptr_, const uint8_t* end_) noexcept : _ptr{ptr_}, _end{end_} {}

  /*!
   * \brief Reads a value from .eh_frame or .eh_frame_hdr section.
   * \param val_ A variable to read the value into.
   * \param enc_ Format of the value.
   * \return \c true if the value has been read successfully and placed in \c val_,
   *         \c false otherwise.
   * \note In case of success, pointer to the DWARF data will be advanced appropriately.
   */
  bool readEhFrameValue(uint64_t& val_, uint8_t enc_) noexcept;

  /*!
   * \return Pointer to the raw DWARF data.
   */
  const uint8_t* getPtr() const noexcept { return _ptr; }

public:
  uint8_t* _ptr;
  const uint8_t* _end;
};

}} // namespace vivify::util
