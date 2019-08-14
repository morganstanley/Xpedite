///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite test for LeasedPtr.
//
// Author: Marcin Dlugajczyk, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////


#include <xpedite/common/LeasedPtr.H>
#include <gtest/gtest.h>

using LeasedPtr = xpedite::common::LeasedPtr<int>;
struct LeasedPtrTest : ::testing::Test
{
};

TEST_F(LeasedPtrTest, StoreValueInEmptyLeasedPtr) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};

  ASSERT_EQ(leasedPtr.provision(std::move(value)).get(), nullptr);
}


TEST_F(LeasedPtrTest, CantStoreIntoOccupiedLeasedPtr) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};
  auto value2{std::make_unique<int>(43)};

  ASSERT_EQ(leasedPtr.provision(std::move(value)), nullptr);
  auto failedProvision = leasedPtr.provision(std::move(value2));
  ASSERT_EQ(*failedProvision, 43);
}


TEST_F(LeasedPtrTest, CantLeaseFromEmptyLeasedPtr) {
  LeasedPtr leasedPtr{};

  ASSERT_EQ(leasedPtr.lease(), nullptr);
}

TEST_F(LeasedPtrTest, CanLeaseFromOccupiedLeasedPtr) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};

  ASSERT_EQ(leasedPtr.provision(std::move(value)), nullptr);
  ASSERT_EQ(*leasedPtr.lease(), 42);
}

TEST_F(LeasedPtrTest, CantLeaseTwice) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};

  ASSERT_EQ(leasedPtr.provision(std::move(value)), nullptr);
  ASSERT_EQ(*leasedPtr.lease(), 42);
  ASSERT_EQ(leasedPtr.lease(), nullptr);
}

TEST_F(LeasedPtrTest, CantStoreIfCurrentValueIsLeaseed) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};
  auto value2{std::make_unique<int>(43)};

  ASSERT_EQ(leasedPtr.provision(std::move(value)), nullptr);
  ASSERT_EQ(*leasedPtr.lease(), 42);
  ASSERT_EQ(*leasedPtr.provision(std::move(value2)), 43);
}

TEST_F(LeasedPtrTest, HasntBeenConsumedIfStored) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};

  ASSERT_EQ(leasedPtr.provision(std::move(value)), nullptr);
  ASSERT_FALSE(leasedPtr.empty());
}

TEST_F(LeasedPtrTest, IsntEmptyIfLeaseReturned) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};

  ASSERT_EQ(leasedPtr.provision(std::move(value)), nullptr);
  leasedPtr.lease();
  ASSERT_FALSE(leasedPtr.empty());
}

TEST_F(LeasedPtrTest, CanReturnAfterLease) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};

  ASSERT_EQ(leasedPtr.provision(std::move(value)), nullptr);
  auto leased = leasedPtr.lease();
  ASSERT_EQ(leasedPtr.returnLease(std::move(leased)), nullptr);
}

TEST_F(LeasedPtrTest, CantReturnIfRevoked) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};

  ASSERT_EQ(leasedPtr.provision(std::move(value)), nullptr);
  auto leased = leasedPtr.lease();
  leasedPtr.revoke();
  auto notReleased = leasedPtr.returnLease(std::move(leased));
  ASSERT_NE(notReleased, nullptr);
  ASSERT_EQ(*notReleased, 42);
}

TEST_F(LeasedPtrTest, CantReleaseOwnershiIfNotLeased) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};

  leasedPtr.provision(std::move(value));
  auto revokeResult = leasedPtr.revoke();
  ASSERT_EQ(*revokeResult, 42);
}

TEST_F(LeasedPtrTest, CanReleaseOwnershiIfLeaseed) {
  LeasedPtr leasedPtr{};
  auto value{std::make_unique<int>(42)};

  leasedPtr.provision(std::move(value));
  auto leased = leasedPtr.lease();
  ASSERT_EQ(leasedPtr.revoke(), nullptr);
}
