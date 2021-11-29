///////////////////////////////////////////////////////////////////////////////
//
// Provides implementation for storing and locating information about pmu events
//
// EventInfo - Stores the name and attributes of the pmu event
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/persistence/SegmentHeader.H>
#include <xpedite/persistence/PackedString.H>
#include <cassert>
#include <sstream>
#include <memory>

namespace xpedite::persistence {

  template<SegmentHeader::Type, typename> class Segment;

  class EventInfo
  {
    template<SegmentHeader::Type, typename> friend class Segment;
    bool _user;
    bool _kernel;
    uint32_t  _eventNameOffset;
    uint32_t  _eventNameSize;
    uint32_t _size;
    char _data[0];

    EventInfo(std::string_view eventName_, bool user_, bool kernel_)
      : _user {user_}, _kernel {kernel_}, _eventNameOffset {}, _size {} {

      _eventNameSize = eventName_.size();
      memcpy(_data+_size, eventName_.data(), eventName_.size());
      _size += eventName_.size();
    }

    EventInfo (const EventInfo&)            = delete;
    EventInfo& operator=(const EventInfo&)  = delete;
    EventInfo (EventInfo&&)                 = delete;
    EventInfo& operator=(EventInfo&&)       = delete;

    public:

    bool user()               const noexcept { return _user;                     }
    bool kernel()             const noexcept { return _kernel;                   }
    const char* eventName()   const noexcept { return _data + _eventNameOffset;  }
    uint32_t eventNameSize()  const noexcept { return _eventNameSize;            }
    size_t size()             const noexcept { return sizeof(EventInfo) + _size; }

    std::string toString() const {
      std::ostringstream os;
      os << "Event {name : " <<  eventName() << ", user :" << user() << ", kernel : " 
        << kernel() << "}";
      return os.str();
    }

  } __attribute__((packed));

  using TopdownNodeInfo = PackedString;
  using MetricInfo = PackedString;

  using Events = std::vector<const persistence::EventInfo*>;
  using EventsPtr = std::shared_ptr<Events>;

  using TopdownNodes = std::vector<const persistence::TopdownNodeInfo*>;
  using TopdownNodesPtr = std::shared_ptr<TopdownNodes>;

  using Metrics = std::vector<const persistence::MetricInfo*>;
  using MetricsPtr = std::shared_ptr<Metrics>;
}

namespace std {
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::persistence::EventInfo& eventInfo_) {
    stream_ << eventInfo_.toString();
    return stream_;
  }
  inline std::ostream& operator<<(std::ostream& stream_, const xpedite::persistence::Events& events_) {
    stream_ << "Events {" << std::endl;
    for(auto* eventInfo : events_) {
      stream_ << "\t\t" << eventInfo->toString();
    }
    stream_ << "}" << std::endl;
    return stream_;
  }
}
