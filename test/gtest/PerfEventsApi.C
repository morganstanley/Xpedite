///////////////////////////////////////////////////////////////////////////////////////////////
//
// Tests for pmu collection using the linux perf events api
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include "PerfEventsApi.H"
#include <xpedite/perf/PerfEventSet.H>
#include <gtest/gtest.h>

namespace xpedite { namespace perf { namespace test {

  struct PerfEventTest : ::testing::Test
  {
  };

  TEST_F(PerfEventTest, BuildAttributes) {
    PerfEventAttrSet attrs {};
    ASSERT_FALSE(attrs) << "failed to detect empty perf event attributes";
    for(int i=0; i<XPEDITE_PMC_CTRL_CORE_EVENT_MAX; ++i) {
      attrs.addPMUEvent(PERF_TYPE_HARDWARE, {}, {}, {});
      ASSERT_TRUE(static_cast<bool>(attrs)) << "failed to add attributes to set";
      ASSERT_EQ(attrs._size, i+1) << "detected mismatch in size of perf event attributes";
    }
    ASSERT_THROW(attrs.addPMUEvent(PERF_TYPE_HARDWARE, {}, {}, {}), std::runtime_error);
  }

  TEST_F(PerfEventTest, BuildEvent) {
    PerfEventsApi api {};
    ASSERT_EQ(api.eventsCount(), 0) << "detected perf events api in invalid state";
    {
      PerfEvent event {{}, {}, -1};
      auto& state = api.lookup(event);
      ASSERT_TRUE(state.isOpen()) << "failed to open event";
      ASSERT_TRUE(static_cast<bool>(event)) << "failed to open event";
      ASSERT_EQ(api.openEventsCount(), 1) << "detected mismatch in open events count";
      ASSERT_EQ(state.groupSize(), 1) << "detected mismatch of events in group";
    }
    ASSERT_EQ(api.eventsCount(), 1) << "detected perf events api in invalid state";
    ASSERT_EQ(api.closedEventsCount(), 1) << "detected perf events api in invalid state";
  }

  void buildEventSet(PerfEventsApi& api_, PerfEventSet& events_, int openEventsCount_) {
    PerfEventSet events;
    ASSERT_FALSE(static_cast<bool>(events)) << "failed to detect empty perf event set";
    ASSERT_EQ(events.groupFd(), -1) << "detected event set with invalid group id";
    for(int i=0; i<XPEDITE_PMC_CTRL_CORE_EVENT_MAX; ++i) {
      PerfEvent event {{}, {}, events.groupFd()};
      auto& state = api_.lookup(event);
      ASSERT_TRUE(static_cast<bool>(event)) << "failed to open event";
      ASSERT_TRUE(state.isOpen()) << "failed to open event";
      ASSERT_EQ(state.groupSize(), i==0) << "detected mismatch of events in group";
      ASSERT_EQ(state.isLeader(), i==0) << "detected failure to tag leader of events";
      ASSERT_EQ(api_.openEventsCount(), openEventsCount_+i+1) << "detected mismatch in open events count";

      events.add(std::move(event));
      ASSERT_TRUE(static_cast<bool>(events)) << "detected mismatch of events state";
      ASSERT_EQ(api_.lookup(events.groupFd()).groupSize(), i+1) << "detected mismatch of events in group";
    }
    ASSERT_THROW(events.add(PerfEvent {}), std::runtime_error);
    ASSERT_FALSE(events.isActive()) << "detected event set in invalid state (expected to INACTIVE)";
    auto* leaderStatePtr = &api_.lookup(events.groupFd());
    ASSERT_NE(leaderStatePtr, nullptr) << "detected invalid group fd";
    ASSERT_EQ(leaderStatePtr->_activationCount, 0) << "detected event with invalid actiation count";
    ASSERT_EQ(leaderStatePtr->_deactivationCount, 0) << "detected event with invalid deactiation count";
    events.activate();
    ASSERT_TRUE(events.isActive()) << "detected failure to activate event set";
    ASSERT_EQ(leaderStatePtr->_activationCount, 1) << "detected event with invalid actiation count";
    ASSERT_TRUE(leaderStatePtr->isActive()) << "detected failure to activate event set";

    events_ = std::move(events);
    ASSERT_FALSE(events.isActive()) << "detected event set in invalid state (expected to INACTIVE)";
    ASSERT_FALSE(static_cast<bool>(events)) << "detected non empty perf event set after move";
  }

  TEST_F(PerfEventTest, BuildEventSet) {
    PerfEventsApi api {};
    PerfEventsApi::EventState* leaderStatePtr {};
    ASSERT_EQ(api.eventsCount(), 0) << "detected perf events api in invalid state";
    {
      PerfEventSet events;
      buildEventSet(api, events, 0);
      leaderStatePtr = &api.lookup(events.groupFd());
    }
    ASSERT_EQ(leaderStatePtr->_deactivationCount, 1) << "detected failure to deactivate event set";
    ASSERT_FALSE(leaderStatePtr->isActive()) << "detected failure to deactivate event set";
    ASSERT_EQ(api.eventsCount(), XPEDITE_PMC_CTRL_CORE_EVENT_MAX) << "detected perf events api in invalid state";
    ASSERT_EQ(api.closedEventsCount(), XPEDITE_PMC_CTRL_CORE_EVENT_MAX) << "detected perf events api in invalid state";
  }

  TEST_F(PerfEventTest, BuildEventSetMap) {
    PerfEventsApi api {};
    const int EVENT_SET_COUNT = 1017;
    int i=0;
    int openEventsCount {};
    ASSERT_EQ(api.eventsCount(), 0) << "detected perf events api in invalid state";
    {
      std::map<pid_t, PerfEventSet> eventSetMap;
      for(; i<EVENT_SET_COUNT; ++i) {
        PerfEventsApi::EventState* leaderStatePtr {};
        {
          PerfEventSet events;
          buildEventSet(api, events, openEventsCount);
          openEventsCount += XPEDITE_PMC_CTRL_CORE_EVENT_MAX;
          leaderStatePtr = &api.lookup(events.groupFd());
          eventSetMap.emplace(std::make_pair(i, std::move(events)));
        }
        ASSERT_EQ(leaderStatePtr->_deactivationCount, 0) << "detected failure to deactivate event set";
        ASSERT_TRUE(leaderStatePtr->isActive()) << "detected failure to deactivate event set";
        ASSERT_EQ(api.closedEventsCount(), 0) << "detected perf events api in invalid state";
      }
    }
    ASSERT_EQ(api.eventsCount(), openEventsCount) << "detected perf events api in invalid state";
    ASSERT_EQ(api.closedEventsCount(), openEventsCount) << "detected perf events api in invalid state";
  }

}}}
