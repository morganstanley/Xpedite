///////////////////////////////////////////////////////////////////////////////
//
// Abstraction to encapsulate API for programming perf events
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/perf/PerfEventsApi.H>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <xpedite/log/Log.H>
#include <xpedite/util/Errno.H>

extern "C" int perf_event_open(const struct perf_event_attr *hwEvent_, pid_t pid_, int cpu_, int groupFd_, unsigned long flags_) {
  return syscall(__NR_perf_event_open, hwEvent_, pid_, cpu_, groupFd_, flags_);
}

namespace xpedite { namespace perf {

  int PerfEventsApi::open(const perf_event_attr* attr_, pid_t pid_, int cpu_, int groupFd_, unsigned long flags_) {
    return perf_event_open(attr_, pid_, cpu_, groupFd_, flags_);
  }

  perf_event_mmap_page* PerfEventsApi::map(int fd_, size_t length_) {
    return reinterpret_cast<perf_event_mmap_page*>(
      mmap(nullptr, length_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0)
    );
  }

  bool PerfEventsApi::unmap(perf_event_mmap_page* addr_, size_t length_) {
    return munmap(addr_, length_) == 0;
  }

  bool PerfEventsApi::close(int fd_) {
    return ::close(fd_) == 0;
  }

  bool PerfEventsApi::enable(int fd_) {
    return ioctl(fd_, PERF_EVENT_IOC_ENABLE, 0) == 0;
  }

  bool PerfEventsApi::reset(int fd_) {
    return ioctl(fd_, PERF_EVENT_IOC_RESET, 0) == 0;
  }

  bool PerfEventsApi::disable(int fd_) {
    return ioctl(fd_, PERF_EVENT_IOC_DISABLE, 0) == 0;
  }

  PerfEventsApi::~PerfEventsApi() {
  }

  PerfEventsApi* PerfEventsApi::DEFAULT_INSTANCE = new PerfEventsApi {};

  PerfEventsApi* PerfEventsApi::_instance {PerfEventsApi::DEFAULT_INSTANCE};

}}
