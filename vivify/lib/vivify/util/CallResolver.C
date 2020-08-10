/*!
 * \file
 * Call Info Resolver class.
 *
 * \author Andrew C., Morgan Stanley
 */

#include <vivify/util/CallResolver.H>

#include <mutex>
#include <cstdlib>
#include <stdexcept>


namespace vivify { namespace util {

namespace
{

#ifndef DMGL_PARAMS
#define DMGL_PARAMS (1u << 0u) // include function arguments
#define DMGL_ANSI   (1u << 1u) // include const, volatile, etc.
#endif

#ifndef NO_BFD_SECTION_FLAGS
inline auto vivify_bfd_section_flags(bfd* /*bfd_*/, asection* section_) noexcept
{
  return bfd_section_flags(section_);
}
inline auto vivify_bfd_section_vma(bfd* /*bfd_*/, asection* section_) noexcept
{
  return bfd_section_vma(section_);
}
inline auto vivify_bfd_section_size(asection* section_) noexcept
{
  return bfd_section_size(section_);
}
#else
inline auto vivify_bfd_section_flags(bfd* bfd_, asection* section_) noexcept
{
  return bfd_get_section_flags(bfd_, section_);
}
inline auto vivify_bfd_section_vma(bfd* bfd_, asection* section_) noexcept
{
  return bfd_get_section_vma(bfd_, section_);
}
inline auto vivify_bfd_section_size(asection* section_) noexcept
{
  return bfd_get_section_size(section_);
}
#endif

struct CallResolverCtxt
{
  bfd* _bfd;
  asymbol** _symTab;

  const bfd_vma _pc;
  const CallResolver::Option _opts;

  bool _stop{false};
  CallInfo _call{};

  CallResolverCtxt(bfd* bfd_, asymbol** symTab_, bfd_vma pc_, CallResolver::Option opts_)
  : _bfd{bfd_}, _symTab{symTab_}, _pc{pc_}, _opts{opts_} {}

  void setInfo(CallInfo::Info& info_, const char* file_, const char* func_) const;
};

void CallResolverCtxt::setInfo(CallInfo::Info& info_, const char* file_, const char* func_) const
{
  if (file_ && '\0' != *file_)
  {
    info_._file = file_;
  }
  if (func_ && '\0' != *func_)
  {
    if (CallResolver::Demangle & _opts)
    {
      char* l_func{bfd_demangle(_bfd, func_, DMGL_ANSI | DMGL_PARAMS)};
      if (l_func)
      {
        try {
          info_._func = l_func;
        } catch (...) {
          free(l_func);
          throw;
        }
        free(l_func);
      }
      else
      {
        info_._func = func_;
      }
    }
    else
    {
      info_._func = func_;
    }
  }
}

void findAddrInSection(bfd* bfd_, asection* section_, void* ctxt_) noexcept
{
  auto& l_ctxt{*static_cast<CallResolverCtxt*>(ctxt_)};

  if (l_ctxt._stop)
  {
    return;
  }
  if (0 == (vivify_bfd_section_flags(bfd_, section_) & SEC_ALLOC))
  { // if not allocated, it is not a debug info section
    return;
  }
  const auto l_vma{vivify_bfd_section_vma(bfd_, section_)};
  if (l_ctxt._pc < l_vma || l_ctxt._pc >= l_vma + vivify_bfd_section_size(section_))
  {
    return;
  }

  l_ctxt._stop = true;

  const char *l_file{nullptr}, *l_func{nullptr};

  auto& l_info{l_ctxt._call._info};
  l_info._valid = bfd_find_nearest_line(
    bfd_, section_, l_ctxt._symTab, (l_ctxt._pc - l_vma),
    &l_file, &l_func, &l_info._line
  );
  l_ctxt.setInfo(l_info, l_file, l_func);
  if ((CallResolver::GetInlineInfo & l_ctxt._opts) && l_info._valid)
  {
    l_file = l_func = nullptr;
    auto& l_inlInfo{l_ctxt._call._inlInfo};
    l_inlInfo._valid = bfd_find_inliner_info(bfd_, &l_file, &l_func, &l_inlInfo._line);
    l_ctxt.setInfo(l_inlInfo, l_file, l_func);
  }
}

} // anonymous namespace


CallResolver::CallResolver(const std::string& file_)
{
  static std::once_flag l_bfdInit;
  std::call_once(l_bfdInit, [](){ bfd_init(); });

  const auto l_bfdErrMsg = [file_](){
    return '\'' + file_ + "': " + bfd_errmsg(bfd_get_error());
  };

  _bfd = bfd_openr(file_.c_str(), nullptr);
  if (!_bfd)
  {
    throw std::runtime_error{"bfd failed to open file " + l_bfdErrMsg()};
  }
  if (bfd_check_format(_bfd, bfd_archive))
  {
    close();
    throw std::runtime_error{"bfd failed to get addresses from archive " + l_bfdErrMsg()};
  }
  {
    char** l_matching;
    if (!bfd_check_format_matches(_bfd, bfd_object, &l_matching))
    {
      if ((bfd_error_file_ambiguously_recognized == bfd_get_error()) && l_matching)
      {
        free(l_matching);
      }
      close();
      throw std::runtime_error{"bfd format does not match an archive " + l_bfdErrMsg()};
    }
  }
  if (bfd_get_file_flags(_bfd) & HAS_SYMS)
  {
    unsigned int l_size;

    auto l_symbCount{
      bfd_read_minisymbols(_bfd, false, reinterpret_cast<void**>(&_symTab), &l_size)
    };

    if (0 == l_symbCount)
    {
      l_symbCount = bfd_read_minisymbols(_bfd, true, reinterpret_cast<void**>(&_symTab), &l_size);
    }
    if (l_symbCount < 0)
    {
      close();
      throw std::runtime_error{"bfd failed to load symbol table for '" + file_ + '\''};
    }
  }
}

CallResolver::~CallResolver()
{
  close();
}

CallResolver::CallResolver(CallResolver&& resolver_) noexcept
: _bfd{resolver_._bfd}, _symTab{resolver_._symTab}
{
  resolver_._bfd = nullptr;
  resolver_._symTab = nullptr;
}

CallResolver& CallResolver::operator=(CallResolver&& resolver_) noexcept
{
  if (this != &resolver_)
  {
    close();

    _bfd = resolver_._bfd;
    _symTab = resolver_._symTab;

    resolver_._bfd = nullptr;
    resolver_._symTab = nullptr;
  }
  return *this;
}

CallInfo CallResolver::getCallInfo(uintptr_t ip_, Option opts_) const noexcept
{
  CallResolverCtxt l_ctxt{_bfd, _symTab, ip_, opts_};
  bfd_map_over_sections(_bfd, findAddrInSection, &l_ctxt);
  return l_ctxt._call;
}

void CallResolver::close() noexcept
{
  if (_symTab)
  {
    free(_symTab);
    _symTab = nullptr;
  }
  if (_bfd)
  {
    bfd_close(_bfd);
    _bfd = nullptr;
  }
}

}} // namespace vivify::util
