/*!
 * \file
 * Memory mapping functionality.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include <memory>
#include <cstring>
#include <cassert>

#include <vivify/AddressSpace.H>
#include <vivify/util/EhFrame.H>
#include <vivify/util/CallResolver.H>


namespace vivify {

struct StackCallInfo;

class File
{
public:
  explicit File(std::string name_);
  ~File();

  File(File&&) = delete;
  File& operator=(File&&) = delete;
  File(const File&) = delete;
  File& operator=(const File&) = delete;

  bool open() noexcept;

  int fd() const noexcept { return _fd; }

  bool hasEhFrame() noexcept;
  const auto& getEhFrame() const noexcept { return _ehFrame; }

#ifndef NO_LIBUNWIND_DEBUG_FRAME
  bool isExecutable() noexcept;
#endif

  void getCallInfo(uintptr_t ip_, StackCallInfo& call_, util::CallResolver::Option opts_);

private:
  const std::string _name;
  int _fd{-1};

  util::EhFrame _ehFrame{};
#ifndef NO_LIBUNWIND_DEBUG_FRAME
  enum {UNKNOWN, NONEXEC, EXEC} _type{UNKNOWN};
#endif

  std::unique_ptr<util::CallResolver> _callResolver;
};

class Map
{
public:
  Map(const AddressSpace::Segment* segment_, File* file_);
  ~Map();

  Map(Map&&);
  Map& operator=(Map&&) noexcept;
  Map(const Map&) = delete;
  Map& operator=(const Map&) = delete;

  const auto& segment() const noexcept { return *_segment; }
  File& file() noexcept { return *_file; }

  auto start()       const noexcept { return _segment->start();  }
  auto end()         const noexcept { return _segment->end();    }
  auto offset()      const noexcept { return _segment->offset(); }
  auto size()        const noexcept { return (end() - start());  }
  const auto& name() const noexcept { return _segment->name();   }

  bool operator<(const Map& map_) const noexcept { return (segment() < map_.segment()); }
  bool operator<(uintptr_t addr_) const noexcept { return (segment() < addr_); }

  bool open() noexcept;

  template <typename T>
  void read(T& value_, size_t offset_) const noexcept
  {
    assert(_cache);
    assert(_cache + offset_ + sizeof(T) <= _cache + size());
    memcpy(&value_, _cache + offset_, sizeof(T));
  }

private:
  uint8_t* _cache{nullptr};

  const AddressSpace::Segment* _segment;
  File* _file;
};
inline bool operator<(uintptr_t addr_, const Map& map_) noexcept
{
  return (addr_ < map_.segment());
}

} // namespace vivify
