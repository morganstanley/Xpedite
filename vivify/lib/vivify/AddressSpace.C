/*!
 * \file
 * Logic to load and search virtual address space of a process.
 *
 * \author Andrew C., Morgan Stanley
 */

#include <vivify/AddressSpace.H>

#include <limits.h>
#include <unistd.h>

#include <cstdio>
#include <stdexcept>
#include <algorithm>

#include <vivify/util/Errno.H>


namespace vivify {

namespace
{

inline bool startsWith(const char* str_, size_t len_, const std::string& prefix_) noexcept
{
  const auto l_len{prefix_.length()};
  return (l_len <= len_ && 0 == strncmp(str_, prefix_.c_str(), l_len));
}

AddressSpace::Segment::Type isAnonymous(const char* name_, size_t len_) noexcept
{
  if (0u == len_ ||
      startsWith(name_, len_, "//anon")     ||
      startsWith(name_, len_, "anon_inode") ||
      startsWith(name_, len_, "/dev/zero"))
  {
    return AddressSpace::Segment::Anonymous;
  }
  if (startsWith(name_, len_, "/anon_hugepage"))
  {
    return AddressSpace::Segment::HugePage;
  }

  return AddressSpace::Segment::None;
}

std::string getUnderlyingFilePath(const std::string& path_)
{
#ifdef PATH_MAX
  constexpr size_t l_bufLen{PATH_MAX};
#else
  constexpr size_t l_bufLen{1024u};
#endif
  char l_buf[l_bufLen];

  const auto l_len{readlink(path_.c_str(), l_buf, l_bufLen)};
  if (l_len < 0)
  {
    using namespace std::string_literals;
    throw std::runtime_error{
      "Failed to get executable file path: "s + util::Errno::get().asString()
    };
  }
  if (0 == l_len)
  {
    throw std::runtime_error{"Failed to get executable file path"};
  }
  if (static_cast<size_t>(l_len) == l_bufLen)
  {
    throw std::runtime_error{"Failed to get executable file path: the read buffer is too small"};
  }
  return {l_buf, static_cast<size_t>(l_len)};
}

} // anonymous namespace


AddressSpace::AddressSpace(pid_t pid_, Option opts_)
{
  char l_buf[8u * 1024u];
  std::string l_path, l_execPath;

  if (pid_ < 0)
  {
    l_execPath = getUnderlyingFilePath("/proc/self/exe");
    l_path = "/proc/self/maps";
  }
  else
  {
    l_path = "/proc/" + std::to_string(pid_);
    l_execPath = getUnderlyingFilePath(l_path + "/exe");
    l_path += "/maps";
  }

  FILE* l_fp{fopen(l_path.c_str(), "r")};
  if (!l_fp)
  {
    throw std::runtime_error{"Failed to open '" + l_path + "': " + util::Errno::get().asString()};
  }

  int  l_namePos;
  char l_perms[4u];
  while(fgets(l_buf, sizeof(l_buf), l_fp))
  {
    Segment l_segment{};

    if (4 != sscanf(l_buf, "%lx-%lx %4c %lx %*x:%*x %*d%n",
                            &l_segment._start, &l_segment._end,
                            &l_perms[0u], &l_segment._offset, &l_namePos))
    {
      fclose(l_fp);
      throw std::runtime_error{"Failed to read '" + l_path + "'"};
    }

    for (; isspace(l_buf[l_namePos]); ++l_namePos) {}

    char* l_name{l_buf + l_namePos};
    auto l_len{strlen(l_name)};
    if (l_len > 0u && '\n' == l_name[l_len - 1u])
    {
      l_name[--l_len] = '\0';
    }

    const auto l_emplaceSegment = [&](){
      l_segment._name = l_name;
      l_segment._readable   = ('r' == l_perms[0u]);
      l_segment._writable   = ('w' == l_perms[1u]);
      l_segment._executable = ('x' == l_perms[2u]);
      l_segment._private    = ('p' == l_perms[3u]);
      _segments.emplace_back(std::move(l_segment));
    };

    try {
      if ((l_segment._type = isAnonymous(l_name, l_len)))
      { // anonymous region
        if (!(IgnoreAnonymousRegions & opts_))
        {
          l_emplaceSegment();
        }
      }
      else if ('[' == l_name[0u])
      { // special region
        if (!(IgnoreSpecialRegions & opts_))
        {
          l_segment._type = Segment::Special;
          l_emplaceSegment();
        }
      }
      else
      { // a file name
        l_segment._type = (l_execPath == l_name ? Segment::Self : Segment::File);
        l_emplaceSegment();
      }
    } catch (...) {
      fclose(l_fp);
      throw;
    }
  }

  fclose(l_fp);

  std::sort(_segments.begin(), _segments.end());
}

const AddressSpace::Segment* AddressSpace::find(uintptr_t addr_) const noexcept
{
  auto l_it{std::upper_bound(_segments.begin(), _segments.end(), addr_)};
  return (_segments.end() != l_it && l_it->start() <= addr_ ? &*l_it : nullptr);
}
AddressSpace::Segment* AddressSpace::find(uintptr_t addr_) noexcept
{
  return const_cast<Segment*>(static_cast<const AddressSpace&>(*this).find(addr_));
}

}// namespace vivify
