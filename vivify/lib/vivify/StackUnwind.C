/*!
 * \file
 * Functionality for offline (aka remote) stack unwinding.
 *
 * \author Andrew C., Morgan Stanley
 */

#include <vivify/StackUnwind.H>

#include <libunwind.h>

#include <set>
#include <map>
#include <algorithm>

#include <vivify/util/Elf.H>
#include "MMap.H"


#ifndef NO_LIBUNWIND_DEBUG_FRAME
extern "C" int UNW_OBJ(dwarf_find_debug_frame)(
  int found_, unw_dyn_info_t* di_, unw_word_t ip_, unw_word_t segbase_, const char* objName_,
  unw_word_t start_, unw_word_t end_
);
#define dwarf_find_debug_frame UNW_OBJ(dwarf_find_debug_frame)
#endif

extern "C" int UNW_OBJ(dwarf_search_unwind_table)(
  unw_addr_space_t as_, unw_word_t ip_, unw_dyn_info_t* di_, unw_proc_info_t* pi_,
  int needUnwindInfo_, void* arg_
);
#define dwarf_search_unwind_table UNW_OBJ(dwarf_search_unwind_table)


namespace vivify {

namespace
{

int unwFindProcInfo(unw_addr_space_t /*as_*/, unw_word_t /*ip_*/, unw_proc_info_t* /*pi_*/,
                    int /*needUnwindInfo_*/, void* /*arg_*/);

void unwPutUnwindInfo(unw_addr_space_t /*as_*/, unw_proc_info_t* /*pi_*/, void* /*arg_*/)
{
}

int unwGetDynInfoListAddr(unw_addr_space_t /*as_*/, unw_word_t* /*dilAddr_*/, void* /*arg_*/)
{
  return -UNW_ENOINFO;
}

int unwAccessMem(unw_addr_space_t /*as_*/, unw_word_t /*addr_*/, unw_word_t* /*valp_*/,
                 int /*write_*/, void* /*arg_*/);

int unwAccessReg(unw_addr_space_t /*as_*/, unw_regnum_t /*regnum_*/, unw_word_t* /*valp_*/,
                 int /*write_*/, void* /*arg_*/);

int unwAccessFpreg(unw_addr_space_t /*as_*/, unw_regnum_t /*num_*/, unw_fpreg_t* /*val_*/,
                   int /*write_*/, void* /*arg_*/)
{
  return -UNW_EINVAL; // unsupported
}

int unwResume(unw_addr_space_t /*as_*/, unw_cursor_t* /*cu_*/, void* /*arg_*/)
{
  return -UNW_EINVAL; // unsupported
}

int unwGetProcName(unw_addr_space_t /*as_*/, unw_word_t /*addr_*/,
                   char* /*bufp_*/, size_t /*bufLen_*/,
                   unw_word_t* /*offp_*/, void* /*arg_*/)
{
  return -UNW_EINVAL; // unsupported
}

unw_accessors_t g_unwAccessors =
{
  .find_proc_info         = unwFindProcInfo,
  .put_unwind_info        = unwPutUnwindInfo,
  .get_dyn_info_list_addr = unwGetDynInfoListAddr,
  .access_mem             = unwAccessMem,
  .access_reg             = unwAccessReg,
  .access_fpreg           = unwAccessFpreg,
  .resume                 = unwResume,
  .get_proc_name          = unwGetProcName,
};

} // anonymous namespace


struct StackUnwind::Ctxt
{
  const StackCtxt* _stack{nullptr};

  const AddressSpace* _addrSpace;

  std::map<std::string, File> _files;
  std::vector<Map> _maps;

  unw_addr_space_t _unwAddrSpace{nullptr};

  explicit Ctxt(const AddressSpace* addrSpace_) : _addrSpace{addrSpace_}
  {
    util::Elf::validateElfVersion();

    _unwAddrSpace = unw_create_addr_space(&g_unwAccessors, 0);
    if (!_unwAddrSpace)
    {
      throw std::runtime_error{"unwind: can't create unwind address space"};
    }
    // address space is expected to be constant, cache flushing is not supported
    unw_set_caching_policy(_unwAddrSpace, UNW_CACHE_GLOBAL);
  }
  ~Ctxt()
  {
    unw_destroy_addr_space(_unwAddrSpace);
    _unwAddrSpace = nullptr;
  }
  void reset() noexcept
  {
    _stack = nullptr;
  }

  Map* findMap(uintptr_t addr_) noexcept;
};
Map* StackUnwind::Ctxt::findMap(uintptr_t addr_) noexcept
{
  auto l_it{std::upper_bound(_maps.begin(), _maps.end(), addr_)};
  if (_maps.end() != l_it && l_it->start() <= addr_)
  {
    return &*l_it;
  }

  const auto* l_segment{_addrSpace->find(addr_)};
  if (!l_segment)
  {
    return nullptr;
  }

  const auto& l_name{l_segment->name()};

  auto* l_file{&_files.emplace(
    std::piecewise_construct, std::forward_as_tuple(l_name), std::forward_as_tuple(l_name))
      .first->second};

  return &*_maps.emplace(l_it, l_segment, l_file);
}

namespace
{

int unwFindProcInfo(unw_addr_space_t as_, unw_word_t ip_, unw_proc_info_t* pi_,
                    int needUnwindInfo_, void* arg_)
{
  auto& l_ctxt{*static_cast<StackUnwind::Ctxt*>(arg_)};

  auto* l_map{l_ctxt.findMap(ip_)};
  if (!l_map)
  {
    return -UNW_EINVALIDIP;
  }

  if (l_map->file().hasEhFrame())
  { //.eh_frame
    using TTableEntry = struct {
      uint32_t _startIpOffset;
      uint32_t _fdeOffset;
    };

    const auto& l_ehFrame{l_map->file().getEhFrame()};

    unw_dyn_info_t l_di{};
    l_di.format           = UNW_INFO_FORMAT_REMOTE_TABLE;
    l_di.start_ip         = l_map->start();
    l_di.end_ip           = l_map->end();
    l_di.u.rti.segbase    = l_map->start() + l_ehFrame.segbase() - l_map->offset();
    l_di.u.rti.table_data = l_map->start() + l_ehFrame._tableData - l_map->offset();
    l_di.u.rti.table_len  = l_ehFrame._fdeCount * sizeof(TTableEntry) / sizeof(unw_word_t);
    if (0 == dwarf_search_unwind_table(as_, ip_, &l_di, pi_, needUnwindInfo_, arg_))
    {
      return UNW_ESUCCESS;
    }
  }

#ifndef NO_LIBUNWIND_DEBUG_FRAME // libunwind was built with --enable-debug-frame
  { //.debug_frame
    unw_dyn_info_t l_di{};
    if (dwarf_find_debug_frame(0, &l_di, ip_,
                               l_map->file().isExecutable() ? 0u : l_map->start(),
                               l_map->name().c_str(), l_map->start(), l_map->end()))
    {
      return dwarf_search_unwind_table(as_, ip_, &l_di, pi_, needUnwindInfo_, arg_);
    }
  }
#endif

  return -UNW_ESTOPUNWIND;
}

int unwAccessMem(unw_addr_space_t /*as_*/, unw_word_t addr_, unw_word_t* valp_,
                 int write_, void* arg_)
{
  if (write_)
  { // write is not supported
    return -UNW_EINVAL;
  }

  auto& l_ctxt{*static_cast<StackUnwind::Ctxt*>(arg_)};

  {
    const uint64_t l_start{l_ctxt._stack->getSPReg()};
    const uint64_t l_end{l_start + l_ctxt._stack->size()};

    if (l_start <= addr_ && addr_ + sizeof(unw_word_t) < l_end)
    {
      *valp_ = *reinterpret_cast<const unw_word_t*>(&l_ctxt._stack->data()[addr_ - l_start]);
      return UNW_ESUCCESS;
    }
  }

  Map* l_map{l_ctxt.findMap(addr_)};
  if (l_map && l_map->open())
  {
    assert(addr_ >= l_map->start());
    l_map->read(*valp_, addr_ - l_map->start());
    return UNW_ESUCCESS;
  }

  return -UNW_EINVAL;
}

int unwAccessReg(unw_addr_space_t /*as_*/, unw_regnum_t regnum_, unw_word_t* valp_,
                 int write_, void* arg_)
{
  if (write_)
  { // write is not supported
    return -UNW_EREADONLYREG;
  }

  auto& l_ctxt{*static_cast<StackUnwind::Ctxt*>(arg_)};
  return (l_ctxt._stack->getRegister(regnum_, *valp_) ? UNW_ESUCCESS : -UNW_EBADREG);
}

} // anonymous namespace


StackUnwind::StackUnwind(const AddressSpace* addrSpace_)
: _ctxt{std::make_unique<StackUnwind::Ctxt>(addrSpace_)}
{
}

StackUnwind::~StackUnwind() = default;

std::vector<uintptr_t> StackUnwind::getIps(const StackCtxt& stack_)
{
  if (!stack_.isValid())
  {
    return {};
  }

  try {
    return getIpsInt(stack_);
  } catch (...) {
    _ctxt->reset();
    throw;
  }
}
std::vector<uintptr_t> StackUnwind::getIpsInt(const StackCtxt& stack_)
{
  _ctxt->_stack = &stack_;

  unw_cursor_t l_cursor;
  auto l_ret{unw_init_remote(&l_cursor, _ctxt->_unwAddrSpace, _ctxt.get())};
  if (l_ret)
  {
    switch (l_ret)
    {
      case UNW_EINVAL:
        throw std::runtime_error{"unwind: only supports local"};
      case UNW_EUNSPEC:
        throw std::runtime_error{"unwind: unspecified error"};
      case UNW_EBADREG:
        throw std::runtime_error{"unwind: register unavailable"};
      default:
        throw std::runtime_error{"unwind: unknown error " + std::to_string(l_ret)};
    }
  }

  std::vector<uintptr_t> l_ips{stack_.getIPReg()};
  while (unw_step(&l_cursor) > 0)
  {
    unw_word_t l_reg;
    unw_get_reg(&l_cursor, UNW_REG_IP, &l_reg);

    if (unw_is_signal_frame(&l_cursor) <= 0)
    { // non-activation frames, see dwfl_frame_pc()
      --l_reg;
    }

    l_ips.push_back(l_reg);
  }

  _ctxt->reset();

  return l_ips;
}

std::vector<StackCallInfo> StackUnwind::getCallInfos(const StackCtxt& stack_, bool getInlineInfo_)
{
  const auto l_ips{getIps(stack_)};
  if (l_ips.empty())
  {
    return {};
  }

  auto l_opts{util::CallResolver::Demangle};
  if (getInlineInfo_)
  {
    l_opts |= util::CallResolver::GetInlineInfo;
  }

  std::vector<StackCallInfo> l_calls(l_ips.size());
  for (size_t i{0u}; i < l_ips.size(); ++i)
  {
    auto l_ip{l_ips[i]};
    auto& l_call{l_calls[i]};

    auto* l_map{_ctxt->findMap(l_ip)};
    assert(l_map);

    {
      const auto& l_segment{l_map->segment()};
      if (!l_segment.isSelf() && l_segment.isExecutable() && !l_segment.isWritable())
      { // .text section of a shared library?
        l_ip = l_ip - l_map->start() + l_map->offset();
      }
    }

    l_map->file().getCallInfo(l_ip, l_call, l_opts);
  }

  return l_calls;
}

} // namespace vivify
