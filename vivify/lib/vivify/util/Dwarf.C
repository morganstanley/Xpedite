/*!
 * \file
 * DWARF utility class implementation.
 *
 * \author Andrew C., Morgan Stanley
 */

#include <vivify/util/Dwarf.H>

#include <dwarf.h>


namespace vivify { namespace util {

namespace
{

template <typename T>
bool readEhFrameValueInt(uint64_t& out_, uint8_t*& ptr_, const uint8_t* end_) noexcept
{
  auto* __attribute__((__may_alias__)) l_ptr{reinterpret_cast<T*>(ptr_)};
  const auto* __attribute__((__may_alias__)) l_end{reinterpret_cast<const T*>(end_)};

  if ((l_ptr + 1u) <= l_end)
  {
    out_ += *l_ptr++;
    ptr_ += sizeof(T);
    return true;
  }

  return false;
}

} // anonymous namespace


bool Dwarf::readEhFrameValue(uint64_t& val_, uint8_t enc_) noexcept
{ // reading .eh_frame_hdr section,
  // https://refspecs.linuxfoundation.org/LSB_3.0.0/LSB-PDA/LSB-PDA.junk/dwarfext.html
  val_ = 0u;

  switch (enc_)
  {
    case DW_EH_PE_omit:
      return true;
    case DW_EH_PE_absptr:
      return readEhFrameValueInt<uintptr_t>(val_, _ptr, _end);
    default:
    break;
  }

  switch (enc_ & 0x70)
  { // the upper 4 bits indicate how the value is to be applied
    case DW_EH_PE_absptr: // value is used with no modification
    break;
    case DW_EH_PE_pcrel:  // value is relative to the current program counter
      val_ = reinterpret_cast<uintptr_t>(_ptr);
    break;
    default:
      return false;
  }

  if (0x00 == (enc_ & 0x07))
  {
    enc_ |= DW_EH_PE_udata4;
  }

  switch (enc_ & 0x0f)
  { // the lower 4 bits indicate format of the value
    case DW_EH_PE_sdata4:
      return readEhFrameValueInt<int32_t>(val_, _ptr, _end);
    case DW_EH_PE_udata4:
      return readEhFrameValueInt<uint32_t>(val_, _ptr, _end);
    case DW_EH_PE_sdata8:
      return readEhFrameValueInt<int64_t>(val_, _ptr, _end);
    case DW_EH_PE_udata8:
      return readEhFrameValueInt<uint64_t>(val_, _ptr, _end);
    default:
    break;
  }

  return false;
}

}} // namespace vivify::util
