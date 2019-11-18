/*!
 * \file
 * Data structures to represent virtual address space of a process.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include <type_traits>
#include <cstdint>
#include <string>
#include <vector>


namespace vivify {

/*!
 * \brief List of segments in virtual address space of a process.
 */
class AddressSpace
{
public:
  /*!
   * \brief Memory segment within the virtual address space of the process.
   */
  struct Segment
  {
    enum Type : uint8_t { None=0x0, Self, File, Special, Anonymous, HugePage };

    std::string _name;
    uintptr_t _start{0u}, _end{0u}, _offset{0u};
    bool _readable{false}, _writable{false}, _executable{false}, _private{false};
    Type _type{None};

    const std::string& name() const noexcept { return _name; };

    auto start()  const noexcept { return _start;  }
    auto end()    const noexcept { return _end;    }
    auto offset() const noexcept { return _offset; }

    bool isReadable()   const noexcept { return _readable;    }
    bool isWritable()   const noexcept { return _writable;    }
    bool isExecutable() const noexcept { return _executable;  }
    bool isPrivate()    const noexcept { return _private;     }
    bool isShared()     const noexcept { return !isPrivate(); }

    /*! \return \c true if segment was mapped from executable of the process. */
    bool isSelf()      const noexcept { return Self == _type; }
    /*! \return \c true if segment was mapped from a file. */
    bool isFile()      const noexcept { return (File == _type || isSelf()); }
    /*!
     * Special region is non-anonymous mapping not associated with a file:
     *   - [heap]         = heap of the program
     *   - [stack]        = stack of the main process
     *   - [stack:1001]   = stack of the thread with thread id 1001
     *   - [vdso]         = "virtual dynamic shared object", the kernel system call handler
     */
    bool isSpecial()   const noexcept { return Special == _type; }
    /*! \return \c true if segment is an anonymous huge page mapping. */
    bool isHugePage()  const noexcept { return HugePage == _type; }
    /*! \return \c true if segment is an anonymous mapping. */
    bool isAnonymous() const noexcept { return (Anonymous == _type || isHugePage()); }
  };

  enum Option : uint8_t {
    None = 0x0,
    IgnoreSpecialRegions = 0x1,   ///< Ignore [stack], [heap], [vdso] etc.
    IgnoreAnonymousRegions = 0x2  ///< Ignore anonymous regions
  };

  /*!
   * \brief Maps virtual address space of a process.
   * \param pid_ Identifier of the process. Will map virtual address space of the current process
   *             if \c pid_ is less than zero. Default value is \c -1.
   * \param opts_ Options of the mapping. Default value is \c Option::None.
   * \see Option enum
   */
  AddressSpace(pid_t pid_ = -1, Option opts_ = Option::None);

  /*!
   * \brief Find a segment for a given memory address.
   * \param addr_ The address to find the segment for.
   * \return Pointer to the segment or \c nullptr if the segment is not found.
   * \see Segment struct
   */
  const Segment* find(uintptr_t addr_) const noexcept;

protected:
  Segment* find(uintptr_t addr_) noexcept;

private:
  std::vector<Segment> _segments;
};

inline bool operator<(const AddressSpace::Segment& l_, const AddressSpace::Segment& r_) noexcept
{
  { return (l_.end() < r_.end()); }
}
inline bool operator<(const AddressSpace::Segment& segment_, uintptr_t addr_) noexcept
{
  return (segment_.end() < addr_);
}
inline bool operator<(uintptr_t addr_, const AddressSpace::Segment& segment_) noexcept
{
  return (addr_ < segment_.end());
}
inline auto operator|(AddressSpace::Option l_, AddressSpace::Option r_) noexcept
{
  using T = std::underlying_type_t<AddressSpace::Option>;
  return static_cast<AddressSpace::Option>(static_cast<T>(l_) | static_cast<T>(r_));
}
inline auto& operator|=(AddressSpace::Option& l_, AddressSpace::Option r_) noexcept
{
  l_ = l_ | r_;
  return l_;
}

} // namespace vivify
