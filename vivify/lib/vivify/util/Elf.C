/*!
 * \file
 * ELF utility class.
 *
 * \author Andrew C., Morgan Stanley
 */

#include <vivify/util/Elf.H>

#include <unistd.h>
#include <gelf.h>

#include <cstring>
#include <stdexcept>

#include <vivify/util/Dwarf.H>


namespace vivify { namespace util {

void Elf::validateElfVersion()
{
  if (EV_NONE == elf_version(EV_CURRENT))
  {
    throw std::runtime_error{"unsupported ELF library version requested"};
  }
}

bool Elf::readEhFrame(int fd_, EhFrame& ehFrame_) noexcept
{
  bool l_ret{false};
  EhFrame l_ehFrame{};

  auto* l_elf{elf_begin(fd_, ELF_C_READ, nullptr)};
  if (l_elf)
  {
    GElf_Ehdr l_elfHdr;
    if (gelf_getehdr(l_elf, &l_elfHdr) &&
        elf_rawdata(elf_getscn(l_elf, l_elfHdr.e_shstrndx), nullptr))
    {
      GElf_Shdr l_secHdr;
      Elf_Scn* l_sec{nullptr};
      while ((l_sec = elf_nextscn(l_elf, l_sec)))
      {
        gelf_getshdr(l_sec, &l_secHdr);
        const auto* l_str{elf_strptr(l_elf, l_elfHdr.e_shstrndx, l_secHdr.sh_name)};
        if (l_str && !strcmp(".eh_frame_hdr", l_str))
        {
          l_ehFrame._offset = l_secHdr.sh_offset;
          break;
        }
      }
    }
    elf_end(l_elf);

    if (l_ehFrame._offset)
    {
      struct EhHdr
      {
        uint8_t _version;
        uint8_t _ehFramePtrEnc;
        uint8_t _fdeCountEnc;
        uint8_t _tableEnc;

        uint64_t _enc[2u];

        uint8_t _data[0u];
      } __attribute__((packed)) l_ehHdr;

      if (sizeof(EhHdr) == pread(fd_, &l_ehHdr, sizeof(EhHdr), l_ehFrame._offset))
      {
        uint64_t l_ehFramePtr;
        util::Dwarf l_dwarf{reinterpret_cast<uint8_t*>(&l_ehHdr._enc[0u]), l_ehHdr._data};
        if (l_dwarf.readEhFrameValue(l_ehFramePtr, l_ehHdr._ehFramePtrEnc) &&
            l_dwarf.readEhFrameValue(l_ehFrame._fdeCount, l_ehHdr._fdeCountEnc))
        {
          l_ehFrame._tableData = (l_dwarf.getPtr() - reinterpret_cast<const uint8_t*>(&l_ehHdr)) +
                                 l_ehFrame._offset;
          ehFrame_ = l_ehFrame;
          l_ret = true;
        }
      }
    }
  }

  return l_ret;
}

bool Elf::isExecutable(int fd_) noexcept
{
  auto* l_elf{elf_begin(fd_, ELF_C_READ, nullptr)};
  if (l_elf)
  {
    GElf_Ehdr l_ehdr{};
    if (gelf_getehdr(l_elf, &l_ehdr))
    {
      return (ET_EXEC == l_ehdr.e_type);
    }
  }

  return false;
}

}} // namespace vivify::util
