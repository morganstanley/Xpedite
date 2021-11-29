///////////////////////////////////////////////////////////////////////////////
//
// Packed string - utility class to persist string in samples file
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////


#pragma once
#include <xpedite/persistence/SegmentHeader.H>
#include <sstream>

namespace xpedite::persistence {

  template<SegmentHeader::Type, typename> class Segment;

  class PackedString
  {
    template<SegmentHeader::Type, typename> friend class Segment;

    uint32_t  _nameOffset;
    uint32_t  _nameSize;
    uint32_t _size;
    char _data[0];

    explicit PackedString(std::string_view name_)
      : _nameOffset {}, _size {} {

      _nameSize = name_.size();
      memcpy(_data+_size, name_.data(), name_.size());
      _size += name_.size();
    }

    PackedString (const PackedString&)            = delete;
    PackedString& operator=(const PackedString&)  = delete;
    PackedString (PackedString&&)                 = delete;
    PackedString& operator=(PackedString&&)       = delete;

    public:

    const char* name()   const noexcept { return _data + _nameOffset;  }
    uint32_t nameSize()  const noexcept { return _nameSize;            }
    size_t size()        const noexcept { return sizeof(PackedString) + _size; }

    std::string toString() const {
      std::ostringstream os;
      os << "Event {name : " <<  name() << "}";
      return os.str();
    }
  } __attribute__((packed));
}

namespace std {
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::persistence::PackedString& packedString_) {
    stream_ << packedString_.toString();
    return stream_;
  }
  inline std::ostream& operator<<(std::ostream& stream_, const std::vector<const xpedite::persistence::PackedString*>& packedStrings_) {
    stream_ << "PackedStrings {" << std::endl;
    for(auto* str : packedStrings_) {
      stream_ << "\t\t" << *str;
    }
    stream_ << "}" << std::endl;
    return stream_;
  }
}
