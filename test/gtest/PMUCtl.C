///////////////////////////////////////////////////////////////////////////////
//
// PMUCtl - Tests for PMU programming and collection logic
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include "PerfEventsApi.H"
#include "../util/LogSupressScope.H"
#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/util/RNG.H>
#include <gtest/gtest.h>
#include <limits>
#include <thread>

namespace xpedite { namespace pmu { namespace test {

  using PerfEventsApi = perf::test::PerfEventsApi;

  struct PMUCtlTest : ::testing::Test
  {
  };

  uint8_t rv() {
    static util::RandomNumberGenerator rng {0, static_cast<int>(std::numeric_limits<uint8_t>::max())};
    return rng.next();
  }

  PMUCtlRequest buildPMURequest(uint8_t fixedEvtCount_ = XPEDITE_PMC_CTRL_FIXED_EVENT_MAX,
      uint8_t gpEvtCount_ = XPEDITE_PMC_CTRL_GP_EVENT_MAX, uint8_t offcoreEvtCount_ = {}) {
    return PMUCtlRequest {
      ._cpu = 0, ._fixedEvtCount = fixedEvtCount_, ._gpEvtCount = gpEvtCount_, ._offcoreEvtCount = offcoreEvtCount_,
      ._fixedEvents = {
        PMUFixedEvent {._ctrIndex = 0, ._user = 1, ._kernel = 1},
        PMUFixedEvent {._ctrIndex = 1, ._user = 1, ._kernel = 1},
        PMUFixedEvent {._ctrIndex = 2, ._user = 1, ._kernel = 1}
      },
      ._gpEvents = {
        PMUGpEvent {._eventSelect = rv(), ._unitMask = rv(), ._user = 1, ._kernel = 1, ._invertCMask = 0, ._counterMask = 0, ._edgeDetect = 0, ._anyThread = 0},
        PMUGpEvent {._eventSelect = rv(), ._unitMask = rv(), ._user = 1, ._kernel = 1, ._invertCMask = 0, ._counterMask = 0, ._edgeDetect = 0, ._anyThread = 0},
        PMUGpEvent {._eventSelect = rv(), ._unitMask = rv(), ._user = 1, ._kernel = 1, ._invertCMask = 0, ._counterMask = 0, ._edgeDetect = 0, ._anyThread = 0},
        PMUGpEvent {._eventSelect = rv(), ._unitMask = rv(), ._user = 1, ._kernel = 1, ._invertCMask = 0, ._counterMask = 0, ._edgeDetect = 0, ._anyThread = 0},
        PMUGpEvent {._eventSelect = rv(), ._unitMask = rv(), ._user = 1, ._kernel = 1, ._invertCMask = 0, ._counterMask = 0, ._edgeDetect = 0, ._anyThread = 0},
        PMUGpEvent {._eventSelect = rv(), ._unitMask = rv(), ._user = 1, ._kernel = 1, ._invertCMask = 0, ._counterMask = 0, ._edgeDetect = 0, ._anyThread = 0},
        PMUGpEvent {._eventSelect = rv(), ._unitMask = rv(), ._user = 1, ._kernel = 1, ._invertCMask = 0, ._counterMask = 0, ._edgeDetect = 0, ._anyThread = 0},
        PMUGpEvent {._eventSelect = rv(), ._unitMask = rv(), ._user = 1, ._kernel = 1, ._invertCMask = 0, ._counterMask = 0, ._edgeDetect = 0, ._anyThread = 0}
      },
      ._offcoreEvents = {0, 0}
    };
  }

  using TestCase = std::function<int(void)>;

  void exercisePerfEvents(PerfEventsApi& api, int threadCount_, int fixedEvtCount_, int gpEvtCount_, TestCase testCase_={}) {
    using perf::test::Override;
    auto buffersGuard = Override::samplesBuffer(threadCount_);
    int beginEventsCount {api.eventsCount()};
    int eventsCount {beginEventsCount + threadCount_ * (fixedEvtCount_ + gpEvtCount_)};
    {
      ASSERT_EQ(api.openEventsCount(), 0) << "detected unexpected open events";
      ASSERT_EQ(api.closedEventsCount(), beginEventsCount) << "detected unexpected closed events";
      PMUCtlRequest request {buildPMURequest(fixedEvtCount_, gpEvtCount_)};
      pmuCtl().enablePerfEvents(request);
    }

    ASSERT_EQ(api.eventsCount(), eventsCount) << "detected perf events api in invalid state";
    ASSERT_EQ(api.closedEventsCount(), beginEventsCount) << "detected premature closing of active events";
    if(testCase_) {
      eventsCount += testCase_();
    }
    ASSERT_EQ(api.eventsCount(), eventsCount) << "detected perf events api in invalid state";
    ASSERT_EQ(api.closedEventsCount(), beginEventsCount) << "detected premature closing of active events";

    pmuCtl().disablePerfEvents();
    ASSERT_EQ(api.closedEventsCount(), beginEventsCount) << "detected premature closing of active events";

    pmuCtl().poll();
    ASSERT_EQ(api.closedEventsCount(), beginEventsCount) << "detected premature closing of active events";

    auto quiesceDurationGuard = Override::quiesceDuration();
    pmuCtl().poll();
    ASSERT_EQ(api.closedEventsCount(), eventsCount) << "detected failure to close inactive events";
  }

  TEST_F(PMUCtlTest, SingleThreadedUsage) {
    LogSupressScope redirectCout;
    PerfEventsApi api {};
    int eventsCount {};
    ASSERT_EQ(api.eventsCount(), eventsCount) << "detected perf events api in invalid state";
    const auto threadCount = 1;
    for(int i=1; i<XPEDITE_PMC_CTRL_FIXED_EVENT_MAX; ++i) {
      for(int j=0; j<XPEDITE_PMC_CTRL_GP_EVENT_MAX; ++j) {
        exercisePerfEvents(api, threadCount, i, j);
        eventsCount += i + j;
        ASSERT_EQ(api.eventsCount(), eventsCount) << "detected perf events api in invalid state";
      }
    }
  }

  TEST_F(PMUCtlTest, MultiThreadedUsage) {
    LogSupressScope redirectCout;
    PerfEventsApi api {};
    int eventsCount {};
    ASSERT_EQ(api.eventsCount(), eventsCount) << "detected perf events api in invalid state";
    const auto threadCount = 7;
    for(int t=2; t<=threadCount; ++t) {
      for(int i=1; i<XPEDITE_PMC_CTRL_FIXED_EVENT_MAX; ++i) {
        for(int j=0; j<XPEDITE_PMC_CTRL_GP_EVENT_MAX; ++j) {
          exercisePerfEvents(api, t, i, j);
          eventsCount += t*(i + j);
          ASSERT_EQ(api.eventsCount(), eventsCount) << "detected perf events api in invalid state";
        }
      }
    }
  }

  void exerciseThread(PerfEventsApi& api, int eventsCount) {
    int beginEventsCount {api.eventsCount()};
    std::thread thread {[]() {framework::initializeThread();}};
    thread.join();
    ASSERT_EQ(api.eventsCount(), beginEventsCount + eventsCount) << "detected mismatch in total events";
    ASSERT_EQ(api.openEventsCount(), beginEventsCount + eventsCount) << "detected premature closing of active events";
  }

  TEST_F(PMUCtlTest, NewThreadsUsage) {
    LogSupressScope redirectCout;
    PerfEventsApi api {};
    ASSERT_EQ(api.eventsCount(), 0) << "detected perf events api in invalid state";
    const auto threadCount = 7;
    exercisePerfEvents(api, 1, XPEDITE_PMC_CTRL_FIXED_EVENT_MAX, XPEDITE_PMC_CTRL_GP_EVENT_MAX,
      [threadCount, &api]() {
        auto newEventsCount {XPEDITE_PMC_CTRL_FIXED_EVENT_MAX + XPEDITE_PMC_CTRL_GP_EVENT_MAX};
        for(int t=0; t<threadCount; ++t) {
          exerciseThread(api, newEventsCount);
        }
        return threadCount * newEventsCount;
      }
    );
  }
}}}
