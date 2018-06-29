///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite test for wait free buffer
//
// This test attempts to exercise the wait free buffer by exchanging data
// between a publisher and consumer thread and checking for consistency
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/common/WaitFreeBufferPool.H>
#include <xpedite/log/Log.H>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <memory>
#include <future>
#include <stdexcept>

void writePayload(int* buffer_, int len_, int value_) {
  for(int i=0; i<len_; ++i) {
    buffer_[i] = value_++;
  }
}

void validatePayload(const int* buffer, int len) {
  int value = buffer[0];
  for(int i=1; i<len; ++i) {
    if(buffer[i] != ++value) {
      throw std::runtime_error {"detected data race while validating payload."};
    }
  }
}

constexpr int BUF_LEN = 1024; // 1 Kib
constexpr int POOL_LEN = 1024; // 1 Kib

void run(int iterCount_) {
  // main thread is used to borrow and write to the buffer from the bufferpool
  // The reader will be spawned in a background thread
  using Pool = xpedite::common::WaitFreeBufferPool<int, BUF_LEN, POOL_LEN>;
  std::unique_ptr<Pool> pool {new Pool{}};
  std::promise<bool> promise;
  auto future = promise.get_future();
  int readCount = 0;
  std::atomic<int> bufCount {iterCount_};

  XpediteLogInfo << "Test for WaitFreeBufferPool : iter count = " << iterCount_ << XpediteLogEnd;

  std::thread t {
    [&pool, &promise, &readCount, &bufCount, iterCount_]() {
      const int* buffer {};
      pool->attachReader();
      XpediteLogInfo << "Reader Thread - exercising empty buffer pool" << XpediteLogEnd;
      for(int i=0; i<1000; ++i) {
        buffer = pool->nextReadableBuffer(buffer);
        if(buffer) {
          throw std::runtime_error {"reader fetched buffer from a pool with no data."};
        }
      }
      promise.set_value(true);

      XpediteLogInfo << "Reader Thread - begin racing with writer ..." << XpediteLogEnd;
      while(readCount < bufCount.load(std::memory_order_relaxed)) {
        buffer = pool->nextReadableBuffer(buffer);
        if(buffer) {
          validatePayload(buffer, BUF_LEN);
          ++readCount;
        }
      }
      pool->detachReader();
      XpediteLogInfo << "Reader Thread - skipped " << iterCount_ - readCount << " out of " << iterCount_ << " blocks." << XpediteLogEnd;
    }
  };

  future.wait();

  XpediteLogInfo << "Writer Thread - begin racing with reader ..." << XpediteLogEnd;

  int* buffer {};
  for(int i=0; i<iterCount_; ++i) {
    buffer = pool->nextWritableBuffer();
    if(i >= POOL_LEN) {
      validatePayload(buffer, BUF_LEN);
    }
    writePayload(buffer, BUF_LEN, i);
  }
  bufCount.store(iterCount_ - pool->overflowCount() -1, std::memory_order_release);
  XpediteLogInfo << "Writer Thread - completed " << iterCount_ << " block writes | overflow " << pool->overflowCount() << XpediteLogEnd;
  t.join();
}

struct WaitFreeBufferPoolTest : ::testing::Test
{
};

TEST_F(WaitFreeBufferPoolTest, ExerciseBufferPool) {
  ASSERT_NO_THROW(run(10000000));
}
