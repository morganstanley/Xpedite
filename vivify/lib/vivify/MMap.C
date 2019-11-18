/*!
 * \file
 * Memory mapping functionality.
 *
 * \author Andrew C., Morgan Stanley
 */

#include "MMap.H"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <limits>

#include <vivify/StackCallInfo.H>
#include <vivify/util/Elf.H>


namespace vivify {

File::File(std::string name_) : _name{std::move(name_)}
{
}

File::~File()
{
  if (_fd > -1)
  {
    close(_fd);
  }
}

bool File::open() noexcept
{
  if (-1 == _fd)
  {
    _fd = ::open(_name.c_str(), O_RDONLY);
    return (-1 != _fd);
  }
  return true;
}

bool File::hasEhFrame() noexcept
{
  constexpr auto INVALID_OFFSET{std::numeric_limits<decltype(_ehFrame._offset)>::max()};

  if (0u == _ehFrame._offset)
  {
    _ehFrame._offset = INVALID_OFFSET;
    return (this->open() && util::Elf::readEhFrame(_fd, _ehFrame));
  }

  return (INVALID_OFFSET != _ehFrame._offset);
}

#ifndef NO_LIBUNWIND_DEBUG_FRAME
bool File::isExecutable() noexcept
{
  if (UNKNOWN == _type)
  {
    if (this->open() && util::Elf::isExecutable(_fd)) { _type = File::EXEC; return true; }
    else { _type = File::NONEXEC; return false; }
  }

  return (EXEC == _type);
}
#endif

void File::getCallInfo(uintptr_t ip_, StackCallInfo& call_, util::CallResolver::Option opts_)
{
  if (!_callResolver)
  {
    _callResolver = std::make_unique<util::CallResolver>(_name);
  }

  call_.util::CallInfo::operator=(_callResolver->getCallInfo(ip_, opts_));
  call_._ip = ip_;
  call_._bfile = _name;
}


Map::Map(const AddressSpace::Segment* segment_, File* file_) : _segment{segment_}, _file{file_}
{
}

Map::Map(Map&& map_) : _cache{map_._cache}, _segment{map_._segment}, _file{map_._file}
{
  map_._cache = nullptr;
}

Map& Map::operator=(Map&& map_) noexcept
{
  if (this != &map_)
  {
    _cache      = map_._cache;
    _segment    = map_._segment;
    _file       = map_._file;
    map_._cache = nullptr;
  }
  return *this;
}

Map::~Map()
{
  if (_cache)
  {
    munmap(_cache, size());
  }
}

bool Map::open() noexcept
{
  bool l_ret{true};

  if (!_cache)
  {
    l_ret = false;
    if (file().open())
    {
      void* l_map{mmap(nullptr, size(), PROT_READ, MAP_PRIVATE, file().fd(), offset())};
      if (MAP_FAILED != l_map)
      {
        _cache = static_cast<uint8_t*>(l_map);
        l_ret = true;
      }
    }
  }

  return l_ret;
}

} // namespace vivify
