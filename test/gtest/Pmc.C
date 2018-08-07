///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite test for performance counter collection
//
// This code validates collection of performance counters from user space.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <xpedite/pmu/EventSet.h>
#include <xpedite/framework/Probes.H>
#include <xpedite/util/Tsc.H>
#include <xpedite/probes/Config.H>
#include <xpedite/util/Util.H>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <fstream>

namespace xpedite { namespace test {

  PMUCtlRequest buildPMUCtlRequest(std::initializer_list<uint8_t> indices_) {
    PMUCtlRequest request {};
    int i = 0;
    for(auto index : indices_) {
      PMUFixedEvent event = {
        ._ctrIndex = index,
        ._user = 1,
        ._kernel = 1,
      };
      request._fixedEvents[i++] = event;
    }
    request._fixedEvtCount = i;
    return request;
  }

  uint64_t readPmc(int index_) {
    uint64_t pmcValue {};
    switch(index_) {
      case 0:
        pmcValue = RDPMC(0x40000000);
        break;
      case 1:
        pmcValue = RDPMC(0x40000001);
        break;
      case 2:
        pmcValue = RDPMC(0x40000002);
        break;
      default:
        throw std::runtime_error {"invalid fixed pmc index"};
    }
    return pmcValue;
  }
}}

using namespace xpedite::test;

struct PMCTest : ::testing::Test
{
  PMCTest() {
    xpedite::util::installFaultHandler();
    auto cpu = 0;
    xpedite::util::pinThisThread(cpu);
  }
};

const char* XPEDITE_DEVICE = "/dev/xpedite";

constexpr uint64_t ZERO {};

typedef PMCTest PMCDeathTest;

TEST_F(PMCDeathTest, EnablePmc) {
  {
    std::ofstream device (XPEDITE_DEVICE, std::ios::binary);
    ASSERT_TRUE(device.is_open()) << "failed to open xpedite device";
    auto pmuCtlRequest = buildPMUCtlRequest({0});
    device.write(reinterpret_cast<const char*>(&pmuCtlRequest), sizeof(pmuCtlRequest));
    device.flush();
    readPmc(0);
    ASSERT_TRUE(device.is_open()) << "failed to open xpedite device";
  }
  ASSERT_DEATH(readPmc(0), ".*terminated by signal \\(11\\).*");
}

TEST_F(PMCTest, ReadInstrCount) {
  std::ofstream device (XPEDITE_DEVICE, std::ios::binary);
  ASSERT_TRUE(device.is_open()) << "failed to open xpedite device";
  auto pmuCtlRequest = buildPMUCtlRequest({0});
  device.write(reinterpret_cast<const char*>(&pmuCtlRequest), sizeof(pmuCtlRequest));
  device.flush();

  ASSERT_TRUE(device.is_open()) << "failed to open xpedite device";

  auto instrCount1 = readPmc(0);
  auto instrCount2 = readPmc(0);
  ASSERT_NE(instrCount1, instrCount2) << "detected invalid instruction count";

  auto coreCycles = readPmc(1);
  ASSERT_EQ(coreCycles, ZERO) << "detected invalid core cycles count, for disabled pmc";

  auto refCycles = readPmc(2);
  ASSERT_EQ(refCycles, ZERO) << "detected invalid ref cycles count, for disabled pmc";
}

TEST_F(PMCTest, ReadCoreCycles) {
  std::ofstream device (XPEDITE_DEVICE, std::ios::binary);
  ASSERT_TRUE(device.is_open()) << "failed to open xpedite device";
  auto pmuCtlRequest = buildPMUCtlRequest({1});
  device.write(reinterpret_cast<const char*>(&pmuCtlRequest), sizeof(pmuCtlRequest));
  device.flush();

  ASSERT_TRUE(device.is_open()) << "failed to open xpedite device";

  auto instrCount = readPmc(0);
  ASSERT_EQ(instrCount, ZERO) << "detected invalid  instruction count, for disabled pmc";

  auto coreCycles1 = readPmc(1);
  auto coreCycles2 = readPmc(1);
  ASSERT_NE(coreCycles1, coreCycles2) << "detected invalid core cycles count";

  auto refCycles = readPmc(2);
  ASSERT_EQ(refCycles, ZERO) << "detected invalid ref cycles count, for disabled pmc";
}

TEST_F(PMCTest, ReadAllFixedPmc) {
  std::ofstream device (XPEDITE_DEVICE, std::ios::binary);
  ASSERT_TRUE(device.is_open()) << "failed to open xpedite device";
  auto pmuCtlRequest = buildPMUCtlRequest({0, 1, 2});
  device.write(reinterpret_cast<const char*>(&pmuCtlRequest), sizeof(pmuCtlRequest));
  device.flush();

  ASSERT_TRUE(device.is_open()) << "failed to open xpedite device";

  auto instrCount1 = readPmc(0);
  auto instrCount2 = readPmc(0);
  ASSERT_NE(instrCount1, instrCount2) << "detected invalid instruction count";

  auto coreCycles1 = readPmc(1);
  auto coreCycles2 = readPmc(1);
  ASSERT_NE(coreCycles1, coreCycles2) << "detected invalid core cycles count";

  auto refCycles1 = readPmc(2);
  auto refCycles2 = readPmc(2);
  ASSERT_NE(refCycles1, refCycles2) << "detected invalid ref cycles count";
}
