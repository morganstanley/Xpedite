///////////////////////////////////////////////////////////////////////////////////////////////
//
// Test for xpedite probe data
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include<xpedite/framework/ProbeData.H>
#include <gtest/gtest.h>

namespace xpedite { namespace framework { namespace test {

  struct ProbeDataTest : ::testing::Test
  {
  };

  TEST_F(ProbeDataTest, InitBytes) {
    ProbeData probeData {
      static_cast<uint8_t>(0), 
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(2),
      static_cast<uint8_t>(3),
      static_cast<uint8_t>(4),
      static_cast<uint8_t>(5),
      static_cast<uint8_t>(6),
      static_cast<uint8_t>(7),
      static_cast<uint8_t>(8),
      static_cast<uint8_t>(9),
      static_cast<uint8_t>(10),
      static_cast<uint8_t>(11),
      static_cast<uint8_t>(12),
      static_cast<uint8_t>(13),
      static_cast<uint8_t>(14),
      static_cast<uint8_t>(15)
    };

    ASSERT_EQ((probeData.get<uint8_t, 0>()), 0)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 1>()), 1)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 2>()), 2)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 3>()), 3)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 4>()), 4)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 5>()), 5)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 6>()), 6)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 7>()), 7)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 8>()), 8)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 9>()), 9)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 10>()), 10) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 11>()), 11) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 12>()), 12) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 13>()), 13) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 14>()), 14) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 15>()), 15) << "detected mismatch in stored uint8_t value";
  }

  TEST_F(ProbeDataTest, LoadStoreBytes) {
    ProbeData probeData {};
    probeData.set<uint8_t, 0>(static_cast<uint8_t>(0));
    probeData.set<uint8_t, 1>(static_cast<uint8_t>(1));
    probeData.set<uint8_t, 2>(static_cast<uint8_t>(2));
    probeData.set<uint8_t, 3>(static_cast<uint8_t>(3));
    probeData.set<uint8_t, 4>(static_cast<uint8_t>(4));
    probeData.set<uint8_t, 5>(static_cast<uint8_t>(5));
    probeData.set<uint8_t, 6>(static_cast<uint8_t>(6));
    probeData.set<uint8_t, 7>(static_cast<uint8_t>(7));
    probeData.set<uint8_t, 8>(static_cast<uint8_t>(8));
    probeData.set<uint8_t, 9>(static_cast<uint8_t>(9));
    probeData.set<uint8_t, 10>(static_cast<uint8_t>(10));
    probeData.set<uint8_t, 11>(static_cast<uint8_t>(11));
    probeData.set<uint8_t, 12>(static_cast<uint8_t>(12));
    probeData.set<uint8_t, 13>(static_cast<uint8_t>(13));
    probeData.set<uint8_t, 14>(static_cast<uint8_t>(14));
    probeData.set<uint8_t, 15>(static_cast<uint8_t>(15));

    ASSERT_EQ((probeData.get<uint8_t, 0>()), 0)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 1>()), 1)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 2>()), 2)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 3>()), 3)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 4>()), 4)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 5>()), 5)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 6>()), 6)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 7>()), 7)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 8>()), 8)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 9>()), 9)   << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 10>()), 10) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 11>()), 11) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 12>()), 12) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 13>()), 13) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 14>()), 14) << "detected mismatch in stored uint8_t value";
    ASSERT_EQ((probeData.get<uint8_t, 15>()), 15) << "detected mismatch in stored uint8_t value";
  }

  TEST_F(ProbeDataTest, InitWords) {
    ProbeData probeData {
      static_cast<uint16_t>(0), 
      static_cast<uint16_t>(1),
      static_cast<uint16_t>(2),
      static_cast<uint16_t>(3),
      static_cast<uint16_t>(4),
      static_cast<uint16_t>(5),
      static_cast<uint16_t>(6),
      static_cast<uint16_t>(7),
    };

    ASSERT_EQ((probeData.get<uint16_t, 0>()), 0)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 1>()), 1)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 2>()), 2)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 3>()), 3)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 4>()), 4)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 5>()), 5)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 6>()), 6)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 7>()), 7)   << "detected mismatch in stored uint16_t value";
  }

  TEST_F(ProbeDataTest, LoadStoreWords) {
    ProbeData probeData {};
    probeData.set<uint16_t, 0>(static_cast<uint16_t>(0));
    probeData.set<uint16_t, 1>(static_cast<uint16_t>(1));
    probeData.set<uint16_t, 2>(static_cast<uint16_t>(2));
    probeData.set<uint16_t, 3>(static_cast<uint16_t>(3));
    probeData.set<uint16_t, 4>(static_cast<uint16_t>(4));
    probeData.set<uint16_t, 5>(static_cast<uint16_t>(5));
    probeData.set<uint16_t, 6>(static_cast<uint16_t>(6));
    probeData.set<uint16_t, 7>(static_cast<uint16_t>(7));

    ASSERT_EQ((probeData.get<uint16_t, 0>()), 0)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 1>()), 1)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 2>()), 2)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 3>()), 3)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 4>()), 4)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 5>()), 5)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 6>()), 6)   << "detected mismatch in stored uint16_t value";
    ASSERT_EQ((probeData.get<uint16_t, 7>()), 7)   << "detected mismatch in stored uint16_t value";
  }

  TEST_F(ProbeDataTest, InitDoubleWords) {
    ProbeData probeData {
      static_cast<uint32_t>(0), 
      static_cast<uint32_t>(1),
      static_cast<uint32_t>(2),
      static_cast<uint32_t>(3),
    };

    ASSERT_EQ((probeData.get<uint32_t, 0>()), static_cast<uint32_t>(0))   << "detected mismatch in stored uint32_t value";
    ASSERT_EQ((probeData.get<uint32_t, 1>()), static_cast<uint32_t>(1))   << "detected mismatch in stored uint32_t value";
    ASSERT_EQ((probeData.get<uint32_t, 2>()), static_cast<uint32_t>(2))   << "detected mismatch in stored uint32_t value";
    ASSERT_EQ((probeData.get<uint32_t, 3>()), static_cast<uint32_t>(3))   << "detected mismatch in stored uint32_t value";
  }

  TEST_F(ProbeDataTest, LoadStoreDoubleWords) {
    ProbeData probeData {};
    probeData.set<uint32_t, 0>(static_cast<uint32_t>(0));
    probeData.set<uint32_t, 1>(static_cast<uint32_t>(1));
    probeData.set<uint32_t, 2>(static_cast<uint32_t>(2));
    probeData.set<uint32_t, 3>(static_cast<uint32_t>(3));

    ASSERT_EQ((probeData.get<uint32_t, 0>()), static_cast<uint32_t>(0))   << "detected mismatch in stored uint32_t value";
    ASSERT_EQ((probeData.get<uint32_t, 1>()), static_cast<uint32_t>(1))   << "detected mismatch in stored uint32_t value";
    ASSERT_EQ((probeData.get<uint32_t, 2>()), static_cast<uint32_t>(2))   << "detected mismatch in stored uint32_t value";
    ASSERT_EQ((probeData.get<uint32_t, 3>()), static_cast<uint32_t>(3))   << "detected mismatch in stored uint32_t value";
  }

  TEST_F(ProbeDataTest, InitQuadWords) {
    ProbeData probeData {
      static_cast<uint64_t>(0), 
      static_cast<uint64_t>(1),
    };

    ASSERT_EQ((probeData.get<uint64_t, 0>()), static_cast<uint64_t>(0))   << "detected mismatch in stored uint64_t value";
    ASSERT_EQ((probeData.get<uint64_t, 1>()), static_cast<uint64_t>(1))   << "detected mismatch in stored uint64_t value";
  }

  TEST_F(ProbeDataTest, LoadStoreQuadWords) {
    ProbeData probeData {};
    probeData.set<uint64_t, 0>(static_cast<uint64_t>(0));
    probeData.set<uint64_t, 1>(static_cast<uint64_t>(1));

    ASSERT_EQ((probeData.get<uint64_t, 0>()), static_cast<uint64_t>(0))   << "detected mismatch in stored uint64_t value";
    ASSERT_EQ((probeData.get<uint64_t, 1>()), static_cast<uint64_t>(1))   << "detected mismatch in stored uint64_t value";
  }

}}}
