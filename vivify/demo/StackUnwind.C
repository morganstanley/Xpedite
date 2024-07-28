/*!
 * \file
 * Example of offline stack unwinding.
 *
 * \author Andrew C., Morgan Stanley
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <linux/perf_event.h>
#include <libunwind-x86_64.h>

#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <algorithm>

#include <vivify/util/Errno.H>
#include <vivify/StackUnwind.H>
#include <vivify/AddressSpace.H>
#include <vivify/StackCallInfoOStream.H>


constexpr size_t PAGE_SIZE{4096};
constexpr size_t DATA_SIZE{PAGE_SIZE};
constexpr size_t MMAP_SIZE{PAGE_SIZE + DATA_SIZE};

struct __attribute__((packed)) Sample
{
  struct perf_event_header _header;

  // see perf_regs.h: PERF_REG_X86_BP, PERF_REG_X86_SP, PERF_REG_X86_IP
  constexpr static uint64_t REGS[] = {1ULL << 6, 1ULL << 7, 1ULL << 8};

  // PERF_SAMPLE_REGS_USER
  const uint64_t _abi;
  const uint64_t _regs[sizeof(REGS)/sizeof(REGS[0])];

  constexpr static uint64_t MAX_STACK_SIZE{2048u};

  //PERF_SAMPLE_STACK_USER
  const uint64_t _size;
  const uint8_t  _data[MAX_STACK_SIZE];
  const uint64_t _dynSize;

  bool isValid() const noexcept { return (_dynSize > 0u && _dynSize <= _size ); }
};
constexpr uint64_t Sample::REGS[];

struct PerfStackCtx : public vivify::StackCtxt
{
  const Sample* _sample{nullptr};

  uint64_t size() const noexcept override { return _sample->_dynSize; }
  const uint8_t* data() const noexcept override { return _sample->_data; }

  uint64_t getSPReg() const noexcept override { return _sample->_regs[1u]; }
  uint64_t getIPReg() const noexcept override { return _sample->_regs[2u]; }
  bool getRegister(unw_regnum_t unwRegNum_, uint64_t& value_) const noexcept override
  {
    switch (unwRegNum_)
    {
      case UNW_X86_64_RBP: value_ = _sample->_regs[0u]; return true;
      case UNW_X86_64_RSP: value_ = getSPReg(); return true;
      case UNW_X86_64_RIP: value_ = getIPReg(); return true;
      default: break;
    }
    return false;
  };
};


void __attribute__ ((noinline)) foo3(int arg_)
{
  while (arg_)
  { // trigger a page fault
    usleep(200000);
    std::vector<uint8_t> l_vector(1024u * 1024u * 1024u);
    l_vector[l_vector.size()/2u] = 42;
  }
}

struct Foo2
{
  static void __attribute__ ((noinline)) foo2(int arg_)
  {
    foo3(arg_);
  }
};

struct Foo1
{
  void __attribute__ ((noinline)) foo1(int arg_) const
  {
    Foo2::foo2(arg_);
  }
};

void __attribute__ ((noinline)) foo0()
{
  Foo1{}.foo1(42);
}

int perf_event_open(struct perf_event_attr* attr_,
                    pid_t pid_, int cpu_, int groupFd_, unsigned long flags_)
{
  return syscall(SYS_perf_event_open, attr_, pid_, cpu_, groupFd_, flags_);
}


int main (int /*argc*/, char** /*argv_*/)
{
  int l_ret{EXIT_SUCCESS};
  pid_t l_child{0};
  int l_fd{-1};

  volatile struct perf_event_mmap_page* l_metaPage{nullptr};
  char* l_dataPage{nullptr};
  size_t l_progress{0u};
  volatile uint64_t l_lastHead{0u};

  PerfStackCtx l_stackCtxt;

  vivify::AddressSpace l_addrSpace{
    -1,
    vivify::AddressSpace::IgnoreAnonymousRegions |
    vivify::AddressSpace::IgnoreSpecialRegions
  };

  vivify::StackUnwind l_stackUnwinder{&l_addrSpace};

  switch ((l_child = fork()))
  {
    case -1:
    {
      fprintf(stderr, "fork failed: %s\n", vivify::util::Errno::get().asString());
      goto error;
    }
    case 0:
    {
      foo0();
      return EXIT_SUCCESS;
    }
    default:
      break;
  }

  sleep(1);

  {
    perf_event_attr l_perfEventAttr{};
    l_perfEventAttr.type = PERF_TYPE_SOFTWARE;          // or PERF_TYPE_HARDWARE...
    l_perfEventAttr.config = PERF_COUNT_SW_PAGE_FAULTS; // ... and PERF_COUNT_HW_CPU_CYCLES
    l_perfEventAttr.size = sizeof(struct perf_event_attr);
    l_perfEventAttr.disabled = 1;
    l_perfEventAttr.exclude_user = 0;
    l_perfEventAttr.exclude_kernel = 1;
    l_perfEventAttr.exclude_hv = 1;
    l_perfEventAttr.sample_type = PERF_SAMPLE_REGS_USER | PERF_SAMPLE_STACK_USER;
    {
      auto& l_regs = l_perfEventAttr.sample_regs_user;
      std::for_each(Sample::REGS, Sample::REGS + sizeof(Sample::REGS)/sizeof(Sample::REGS[0]),
                    [&l_regs](uint64_t reg_){ l_regs |= reg_; });
    }
    l_perfEventAttr.sample_stack_user = Sample::MAX_STACK_SIZE;
    l_perfEventAttr.sample_period = 10u;

    if ((l_fd = perf_event_open(&l_perfEventAttr, l_child, -1, -1, 0)) < 0)
    {
      fprintf(stderr, "perf_event_open failed: %s\n", vivify::util::Errno::get().asString());
      goto error;
    }
  }

  l_metaPage = (struct perf_event_mmap_page*)mmap(
    nullptr, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, l_fd, 0
  );
  if (MAP_FAILED == l_metaPage)
  {
    fprintf(stderr, "mmap failed: %s\n", vivify::util::Errno::get().asString());
    goto error;
  }
  if (ioctl(l_fd, PERF_EVENT_IOC_ENABLE))
  {
    fprintf(stderr, "ioctl failed: %s\n", vivify::util::Errno::get().asString());
    goto error;
  }

  l_dataPage = ((char*)l_metaPage) + PAGE_SIZE;

  for (size_t i{0u}; i < 1u;)
  {
    while (l_metaPage->data_head == l_lastHead) {}
    l_lastHead = l_metaPage->data_head;

    while (l_progress < l_lastHead && i < 1u)
    {
      const auto* l_sample{(struct Sample*)(l_dataPage + l_progress % DATA_SIZE)};
      switch (l_sample->_header.type)
      {
        case PERF_RECORD_SAMPLE:
        {
          if (l_sample->_header.size < sizeof(struct Sample))
          {
            fprintf(stderr, "sample size is too small.\n");
            goto error;
          }

          if (l_sample->isValid())
          {
            l_stackCtxt._sample = l_sample;

            { // ips only
              fprintf(stdout, "ips: ");
              for (const auto& l_ip: l_stackUnwinder.getIps(l_stackCtxt))
              {
                fprintf(stdout, "0x%lx ", l_ip);
              }
              fprintf(stdout, "\n\n");
            }
            { // complete call chain
              const auto l_callers{l_stackUnwinder.getCallInfos(l_stackCtxt, true)};
              std::ostringstream l_stream;
              l_stream << l_callers;
              fprintf(stdout, "%s\n", l_stream.str().c_str());
            }
          }

          ++i;

          break;
        }
        case PERF_RECORD_THROTTLE:
        case PERF_RECORD_UNTHROTTLE:
        case PERF_RECORD_LOST:
          break;
        default:
          fprintf(stderr, "unexpected event: %x\n", l_sample->_header.type);
          goto error;
      }

      l_progress += l_sample->_header.size;
    }

    l_metaPage->data_tail = l_lastHead;
  }
  goto cleanup;

error:
  l_ret = EXIT_FAILURE;
cleanup:
  if (l_child) kill(l_child, SIGKILL);
  if (l_fd > 0) close(l_fd);

  return l_ret;
}
