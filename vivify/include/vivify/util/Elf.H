/*!
 * \file
 * ELF utility class.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include "EhFrame.H"


namespace vivify { namespace util {

/*!
 * \brief ELF utility class.
 */
class Elf
{
public:
  Elf() = delete;

  /*!
   * \brief Validate ELF library operating version.
   * \throw std::runtime_error if version of the library is invalid.
   */
  static void validateElfVersion();

  /*!
   * \brief Read .eh_frame data.
   * \param fd_ Open descriptor of a file to read the data from.
   * \param ehFrame_ Data structure to read the data into.
   * \return \c true if the data has been read successfully, \c false otherwise.
   * \see EhFrame struct
   */
  static bool readEhFrame(int fd_, EhFrame& ehFrame_) noexcept;

  /*!
   * \brief Detect if file is an executable.
   * \param fd_ Open descriptor of the file.
   * \return \c true if the file is detected as executable, \c false otherwise.
   */
  static bool isExecutable(int fd_) noexcept;
};

}} // namespace vivify::util
