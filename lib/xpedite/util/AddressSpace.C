///////////////////////////////////////////////////////////////////////////////
//
// Logic to load and search process address space
//
// Segment - represents a block of memory mapped by the target processes
//
// AddressSpace - List of segments in a processes address space
//
// Provides logic to locate code segments containing probes.
// The page protections are updated during probe activation/deactivation.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/util/AddressSpace.H>
#include <sstream>
#include <fstream>
#include <iostream>

namespace xpedite { namespace util {

  AddressSpace* AddressSpace::_instance;

  void AddressSpace::Segment::toString(std::ostringstream& os_) const {
    os_ << "Segment [" << this << "] {" << 
        std::hex << static_cast<const void*>(_begin) << "-" << static_cast<const void*>(_end) << " | " << std::dec <<
        "can read - " << canRead() << ", can write - " << canWrite() << ", can exec - " << canExec() <<
      "}";
  }

  std::string AddressSpace::Segment::toString() const {
    std::ostringstream os;
    toString(os);
    return os.str();
  }

  AddressSpace::Segment readSegment(std::string record) {
    std::string range, flags;
    {
      std::istringstream stream {record};
      if(!(stream >> range >> flags)) { 
        return {};
      }
    }
    std::istringstream stream {range};
    std::string begin, end;
    if(std::getline(stream, begin, '-') && std::getline(stream, end, ' ')) {
      auto b = reinterpret_cast<AddressSpace::Segment::Pointer>(std::stoull(begin, 0, 16));
      auto e = reinterpret_cast<AddressSpace::Segment::Pointer>(std::stoull(end, 0, 16));
      return AddressSpace::Segment {b, e, flags[0] == 'r', flags[1] == 'w', flags[2] == 'x'};
    }
    return {};
  }

  AddressSpace::AddressSpace()
    : _segments {load()} {
  }

  std::string AddressSpace::toString() const noexcept {
    std::ostringstream os;
    for(auto& segment : _segments) {
      segment.toString(os);
      os << std::endl;
    }
    return os.str();
  }

  AddressSpace::Segments AddressSpace::load() {
    Segments segments;
    std::ifstream pmap {"/proc/self/maps"};
    std::string line;
    while (std::getline(pmap, line)) {
      if(auto segment = readSegment(line)) {
        segments.emplace_back(segment);
      }
    }
    return segments;
  }

  AddressSpace::Segment* AddressSpace::find(AddressSpace::Segment::ConstPointer addr_) noexcept {
    for(auto& segment : _segments) {
      if(segment.begin() <= addr_ && addr_ < segment.end()) {
        return &segment;
      }
    }
    return {};
  }

  const AddressSpace::Segment* AddressSpace::find(AddressSpace::Segment::ConstPointer addr_) const noexcept {
    return const_cast<AddressSpace*>(this)->find(addr_);
  }
}}
