/*!
 * \file
 * Thread safe functionality to access errno and its string description.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include <cstring>
#include <array>
#include <cerrno>


namespace vivify { namespace util {

/*!
 * \brief Thread safe class to access errno and its string description.
 */
struct Errno
{
  /*!
   * \return Number of last error.
   * \see errno variable.
   */
  static int lastErrorCode() noexcept
  {
    return errno;
  }

  /*!
   * \return String describing last error number.
   * \see lastErrorCode()
   */
  const char* asString() const noexcept
  {
    return strerror_r(lastErrorCode(), _buffer.data(), _buffer.size());
  }

  /*!
   * \return Reference to thread local instance of Errno class.
   * \see Errno class
   */
  static const Errno& get() noexcept
  {
    static thread_local Errno l_errno;
    return l_errno;
  }

private:
  // The GNU C Library uses a buffer of 1024 characters for strerror().
  mutable std::array<char, 1024u> _buffer;
};

}} // namespace vivify::util
