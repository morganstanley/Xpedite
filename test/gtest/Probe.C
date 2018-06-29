///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite test for probe activation and de-activation
//
// This test exercises the following.
//  1. Activates probe and validates instruction at callsite
//  2. Deactivates probe and validates instruction at callsite
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/probes/Probe.H>
#include <unistd.h>
#include <gtest/gtest.h>

namespace xpedite { namespace probes { namespace test {

  CallSite callSite(void* ptr_) {
    return reinterpret_cast<CallSite>(const_cast<volatile void*>(ptr_));
  }

  struct ProbeTest : ::testing::Test
  {
    static Probe buildProbe(void* callSite_) {
      Probe probe {};
      probe._callSite = callSite(callSite_);
      probe._name = "TestProbe";
      probe._file = __FILE__;
      probe._func = __PRETTY_FUNCTION__;
      probe._next = nullptr;
      probe._prev = nullptr;
      probe._line = __LINE__;
      probe._id = 0;
      probe._attr = {};
      return probe;
    }
  };

  constexpr int PMU_RECORDER_INDEX {2};

  TEST_F(ProbeTest, ProbeValidation) {
    alignas(8) unsigned char buffer[getpagesize()] {};
    Probe probe {ProbeTest::buildProbe(buffer+1)};
    ASSERT_FALSE(probe.isValid(callSite(buffer+1), callSite(buffer+6))) << "falied to detect unaligned probe";

    probe = ProbeTest::buildProbe(buffer);
    ASSERT_FALSE(probe.isValid(callSite(buffer), callSite(buffer))) << "falied to detect invalid call site size";
    ASSERT_FALSE(probe.isValid(callSite(buffer+1), callSite(buffer+6))) << "falied to detect probe with mismatching call site";
    ASSERT_FALSE(probe.isValid(callSite(buffer), callSite(buffer+5))) << "falied to detect non NOP Instructions at call site";
    memcpy(buffer, &FIVE_BYTE_NOP, sizeof(FIVE_BYTE_NOP));
    ASSERT_TRUE(probe.isValid(callSite(buffer), callSite(buffer+5))) << "detected misvalidation of valid probe";
  }


  TEST_F(ProbeTest, ProbeActivation) {
    unsigned char buffer[getpagesize()] {};
    Probe probe {ProbeTest::buildProbe(nullptr)};
    ASSERT_FALSE(probe.canActivate()) << "falied to detect null call site";

    probe = ProbeTest::buildProbe(buffer);
    ASSERT_TRUE(probe.canActivate()) << "falied to detect activatable probe";

    memcpy(buffer, &FIVE_BYTE_NOP, sizeof(FIVE_BYTE_NOP));
    for(unsigned i=5; i<sizeof(buffer); ++i) {
      buffer[i] = i % 256;
    }

    ASSERT_TRUE(probe.isValid(callSite(buffer), callSite(buffer+5))) << "detected misvalidation of opcode at call site";
    ASSERT_TRUE(probe.isValid(callSite(buffer), callSite(buffer+5))) << "detected misvalidation of offset at call site";
    ASSERT_EQ(recorderCtl().activeRecorderIndex(), 0) << "detected invalid initial recorder index";

    ASSERT_FALSE(probe.isActive()) << "detected failure to activate probe";
    ASSERT_FALSE(recorderCtl().activateRecorder(1024)) << "falied to detect invalid recorder";
    recorderCtl().enableGenericPmc(4);
    ASSERT_TRUE(recorderCtl().activeRecorderIndex() == PMU_RECORDER_INDEX) << "detected failure to activate recorder";
    ASSERT_TRUE(probe.activate()) << "falied to detect activatable probe";
    ASSERT_TRUE(probe.isActive()) << "detected failure to activate probe";
    ASSERT_EQ(buffer[0], OPCODE_CALL) << "detected invalid opcode at call site for active probe";

    uint32_t offset {};
    memcpy(&offset, buffer+1, sizeof(offset));
    uint32_t expectedOffset = reinterpret_cast<unsigned char*>(xpediteRecorderTrampoline) - buffer - CAll_SITE_LEN;
    ASSERT_EQ(offset, expectedOffset) << "detected invalid offset at call site for active probe";
    for(unsigned i=5; i<sizeof(buffer); ++i) {
      ASSERT_EQ(buffer[i], i % 256) << "detected corruption of memory";
    }

    probe.deactivate();
    ASSERT_FALSE(probe.isActive()) << "detected failure to deactivate probe";
    ASSERT_EQ(memcmp(FIVE_BYTE_NOP, buffer, sizeof(FIVE_BYTE_NOP)), 0) << "detected invalid opcode at call site for deactivate probe";
    for(unsigned i=5; i<sizeof(buffer); ++i) {
      ASSERT_EQ(buffer[i], i % 256) << "detected corruption of memory";
    }
  }
}}}
