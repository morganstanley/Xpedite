///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite framework initialization and shutdown test
//
// This test ensures a background thread is started and stopped by initialize
// and halt methods respectively
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/util/ThreadInfo.H>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

struct FrameworkTest : ::testing::Test
{
};

TEST_F(FrameworkTest, InitAndShutdown) {

  ASSERT_FALSE(xpedite::framework::isRunning()) << "xpedite framework is already up";

  std::vector<pid_t> childrenPreInit = xpedite::util::getChildren();

  ASSERT_TRUE(xpedite::framework::initialize("xpedite-appinfo.txt")) << "failed to initialize xpedite - aborting";

  std::vector<pid_t> childrenPostInit = xpedite::util::getChildren();

  EXPECT_EQ(childrenPreInit.size() + 1, childrenPostInit.size()) << "xpedite initialize failed to spawn offload thread";

  xpedite::framework::pinThread(0);

  EXPECT_TRUE(xpedite::framework::halt()) << "xpedite framework failed to halt offload thread";

  std::vector<pid_t> childrenPostHalt;
  for(unsigned i=0; i<5; ++i) {
    childrenPostHalt = xpedite::util::getChildren();
    if(childrenPreInit.size() == childrenPostHalt.size()) {
      break;
    }
    std::cout << "awaiting xpedite offload thread halt ..." << std::endl;
    std::this_thread::sleep_for(std::chrono::duration<unsigned, std::milli> {5000});
  }

  EXPECT_EQ(childrenPreInit.size(), childrenPostHalt.size()) << "xpedite initialize failed to join offload thread";
  
}
