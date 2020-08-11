/*!
 * \file
 * Call Info Resolver class.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

// workaround for potential issues with including bfd.h
// https://sourceware.org/bugzilla/show_bug.cgi?id=14243
// https://sourceware.org/bugzilla/show_bug.cgi?id=15920
#ifndef PACKAGE
#define PACKAGE
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION
#endif
#include <bfd.h>

#include <type_traits>

#include "CallInfo.H"


namespace vivify { namespace util {

/*!
 * \brief Call Resolver class.
 * \note Requires libbfd.
 */
class CallResolver
{
public:
  enum Option : uint8_t {
    None          = 0x0,
    GetInlineInfo = 0x1,  ///< Retrieve also <em>inlined by</em> and <em>inlined at</em> info.
    Demangle      = 0x2,  ///< Demangle function names.
    All           = GetInlineInfo | Demangle
  };

  /*!
   * \brief Constructor.
   * \param file_ A file path to the binary file.
   */
  explicit CallResolver(const std::string& file_);
  ~CallResolver();

  CallResolver(CallResolver&&) noexcept;
  CallResolver& operator=(CallResolver&&) noexcept;

  CallResolver(const CallResolver&) = delete;
  CallResolver& operator=(const CallResolver&) = delete;

  /*!
   * \brief Retrieve call info.
   * \param ip_ Instruction pointer.
   * \param opts_ Options. Default value is \c Option::All.
   * \return Call info.
   * \see Option enum.
   * \see CallInfo struct.
   */
  CallInfo getCallInfo(uintptr_t ip_, Option opts_ = Option::All) const noexcept;

private:
  bfd* _bfd{nullptr};
  asymbol** _symTab{nullptr};

  void close() noexcept;
};

inline auto operator|(CallResolver::Option l_, CallResolver::Option r_) noexcept
{
  using T = std::underlying_type_t<CallResolver::Option>;
  return static_cast<CallResolver::Option>(static_cast<T>(l_) | static_cast<T>(r_));
}
inline auto& operator|=(CallResolver::Option& l_, CallResolver::Option r_) noexcept
{
  l_ = l_ | r_;
  return l_;
}

}} // namespace vivify::util
