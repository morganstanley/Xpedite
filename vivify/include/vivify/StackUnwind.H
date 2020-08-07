/*!
 * \file
 * Functionality for offline (aka remote) stack unwinding.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include <vector>
#include <memory>

#include "StackCallInfo.H"


namespace vivify {

class AddressSpace;

/*!
 * \brief Context of a stack to unwind.
 * \note Requires libelf and libunwind.
 */
class StackCtxt
{
public:
  virtual ~StackCtxt() = default;

  virtual const uint8_t* data() const noexcept = 0;  ///< Pointer to a buffer with the stack.
  virtual uint64_t size() const noexcept = 0;        ///< Size of the stack buffer.

  virtual uint64_t getSPReg() const noexcept = 0;    ///< Get value of SP register.
  virtual uint64_t getIPReg() const noexcept = 0;    ///< Get value of IP register.
  /*!
   * \brief Get value of a register.
   * \param unwRegNum_[in] Register enumeration. See libunwind x86_64_regnum_t enum for more info.
   * \param value_[out] A variable to put the register value into.
   * \return \c true if \c reg_ was populated with a value of the register \c unwRegNum_,
   *         \c false otherwise.
   */
  virtual bool getRegister(int /*unwRegNum_*/, uint64_t& /*value_*/) const noexcept
  {
    return false;
  };

  bool isValid() const noexcept { return (size() > 0u && data()); }

protected:
  StackCtxt() = default;
};

/*!
 * \brief Provides functionality to remotely unwinding stack of a process.
 */
class StackUnwind
{
public:
  /*! \cond */
  struct Ctxt;
  /*! \endcond */

  /*!
   * \param addrSpace_ Pointer to the virtual address space of the process.
   * \see AddressSpace class.
   */
  explicit StackUnwind(const AddressSpace* addrSpace_);
  ~StackUnwind();

  /*!
   * \brief Unwind stack.
   * \param stack_ Context of the stack.
   * \return Vector of instruction pointers that represents the call chain.
   * \see StackCtxt class.
   */
  std::vector<uintptr_t> getIps(const StackCtxt& stack_);
  /*!
   * \brief Unwind stack.
   * \param stack_ Context of the stack.
   * \param getInlineInfo_ If \c true and caller was inlined, retrieve also
   *                       <em>inlined by</em> and <em>inlined at</em> info.
   * \return Vector of call infos that represents the call chain.
   * \see StackCtxt class.
   * \see StackCallInfo struct.
   */
  std::vector<StackCallInfo> getCallInfos(const StackCtxt& stack_, bool getInlineInfo_ = false);

private:
  std::unique_ptr<Ctxt> _ctxt;

  std::vector<uintptr_t> getIpsInt(const StackCtxt& stack_);
};

} // namespace vivify
