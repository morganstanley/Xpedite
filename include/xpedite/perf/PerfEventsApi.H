///////////////////////////////////////////////////////////////////////////////
//
// Abstraction to encapsulate API for programming perf events
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <unistd.h>

struct perf_event_attr;
struct perf_event_mmap_page;

namespace xpedite { namespace perf {

  namespace test {
    struct Override;
  }

  class PerfEventsApi {

    static PerfEventsApi* DEFAULT_INSTANCE;

    static PerfEventsApi* _instance;

    friend struct test::Override;

    public:

    virtual int open(const perf_event_attr* attr_, pid_t pid_, int cpu_, int groupFd_, unsigned long flags_);

    virtual perf_event_mmap_page* map(int fd_, size_t length_);

    virtual bool unmap(perf_event_mmap_page* addr_, size_t length_);

    virtual bool enable(int fd_);

    virtual bool reset(int fd_);

    virtual bool disable(int fd_);

    virtual bool close(int fd_);

    virtual ~PerfEventsApi();

    static PerfEventsApi* instance() {
      return _instance;
    }

  };

  inline PerfEventsApi* perfEventsApi() {
    return PerfEventsApi::instance();
  }

}}
