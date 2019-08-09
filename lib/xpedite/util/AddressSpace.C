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

constexpr int HUGE_PAGE_SIZE = 2 * 1024 * 1024;

namespace xpedite { namespace util {

  AddressSpace* AddressSpace::_instance;

  void AddressSpace::Segment::toString(std::ostringstream& os_) const {
    os_ << "Segment [" << this << "] {" << 
        std::hex << static_cast<const void*>(_begin) << "-" << static_cast<const void*>(_end) << " | " << std::dec <<
        "can read - " << canRead() << ", can write - " << canWrite() << ", can exec - " << canExec() <<
        ", is position independent - " << isPositionIndependent() << ", is hugepage - " << isHugePage() << ", file - " << file() <<
      "}";
  }

  std::string AddressSpace::Segment::toString() const {
    std::ostringstream os;
    toString(os);
    return os.str();
  }

  const char* anonymousSegment {"[anonymous]"};
  const char* hugePageSegment {"hugepage"};

  bool isMappingHugePage(unsigned long long size_, const std::string& file_) {
    return !(size_ % HUGE_PAGE_SIZE) && file_.find(hugePageSegment) != std::string::npos;
  }

  AddressSpace::Segment readSegment(std::string record, const std::string& executablePath_) {
    std::string range, flags, file;
    {
      std::istringstream stream {record};
      if(!(stream >> range >> flags)) { 
        return {};
      }
      while(stream) {
        stream >> file;
      }
      if(file[0] != '/' && file[0] != '[') {
        // Anonymous memory segment
        file = anonymousSegment;
      }
    }

    bool isPositionIndependent {file != executablePath_ && file != anonymousSegment};
    std::istringstream stream {range};
    std::string begin, end;
    if(std::getline(stream, begin, '-') && std::getline(stream, end, ' ')) {
      auto b = reinterpret_cast<AddressSpace::Segment::Pointer>(std::stoull(begin, 0, 16));
      auto e = reinterpret_cast<AddressSpace::Segment::Pointer>(std::stoull(end, 0, 16));
      auto isHugePage = isMappingHugePage(e - b, record);
      return AddressSpace::Segment {b, e, flags[0] == 'r', flags[1] == 'w', flags[2] == 'x', isPositionIndependent, isHugePage, file};
    }
    return {};
  }

  AddressSpace::AddressSpace()
    : _executablePath {util::getExecutablePath()}, _segments {load(_executablePath)} {
  }

  std::string AddressSpace::toString() const noexcept {
    std::ostringstream os;
    for(auto& segment : _segments) {
      segment.toString(os);
      os << std::endl;
    }
    return os.str();
  }

  AddressSpace::Segments AddressSpace::load(const std::string& executablePath_) {
    Segments segments;
    std::ifstream pmap {"/proc/self/maps"};
    std::string line;
    while (std::getline(pmap, line)) {
      if(auto segment = readSegment(line, executablePath_)) {
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
