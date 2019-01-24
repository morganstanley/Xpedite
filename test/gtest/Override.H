///////////////////////////////////////////////////////////////////////////////////////////////
//
// An utilty class to support overriding internal state of pmu logic.
// Used to implement intrusive unit tests
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/perf/PerfEventsApi.H>
#include <vector>
#include <functional>
#include <algorithm>
#include <thread>

namespace xpedite { namespace perf { namespace test {

  struct Override
  {
    using SamplesBuffer = framework::SamplesBuffer;

    class Guard
    {
      friend struct Override;

      using Reclaimer = std::function<void(void)>;

      Reclaimer _reclaimer;

      explicit Guard(Reclaimer reclaimer_)
        : _reclaimer {std::move(reclaimer_)} {
      }

      public:

      Guard(const Guard&) = delete;
      Guard& operator=(const Guard&) = delete;
      Guard(Guard&&) = default;
      Guard& operator=(Guard&&) = default;

      ~Guard() {
        _reclaimer();
      }
    };

    static Guard perfEventsApi(PerfEventsApi* instance_) {
      PerfEventsApi::_instance = instance_;
      return Guard {[]() {PerfEventsApi::_instance = PerfEventsApi::DEFAULT_INSTANCE;}};
    }

    static Guard quiesceDuration(uint64_t quiesceDuration_ = 1) {
      pmu::PmuCtl::_quiesceDuration = quiesceDuration_;
      return Guard {[]() {pmu::PmuCtl::_quiesceDuration = pmu::PmuCtl::DEFAULT_QUIESCE_DURATION;}};
    }
    
    static SamplesBuffer* resetBuffers(SamplesBuffer* buffer_) {
      SamplesBuffer* head = SamplesBuffer::_head.load(std::memory_order_relaxed);
      do {
      } while(!SamplesBuffer::_head.compare_exchange_weak(head, buffer_, std::memory_order_release, std::memory_order_relaxed));
      return head;
    }

    static void allocateBuffers(int count_) {
      std::vector<std::thread> threads {};
      for(int i=0; i<count_; ++i) {
        threads.emplace_back(std::thread {[]() {framework::initializeThread();}});
      }
      std::for_each(threads.begin(), threads.end(), [](std::thread& t_){t_.join();});
    }

    static Guard samplesBuffer(int count_) {
      framework::initializeThread(); // current thread
      auto head = resetBuffers(nullptr);

      allocateBuffers(count_);

      return Guard {[head]() {
        auto buffer = resetBuffers(head);
        while(buffer) {
          auto next = buffer->next();
          SamplesBuffer::deallocate(buffer);
          buffer = next;
        }
      }};
    }

  };

}}}
