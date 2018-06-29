///////////////////////////////////////////////////////////////////////////////////////////////
//
// A single use barrier to detect completion of an asynchronous task
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <mutex>
#include <condition_variable>

class Latch
{
  public:

  Latch(unsigned count_)
    : _mutex {}, _cv {}, _count {count_} {
  }

  void countDown() {
    std::lock_guard<std::mutex> lock {_mutex};
    if(_count) {
      if(--_count == 0) {
        _cv.notify_one();
      }
    }
  }

  void wait() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait(lock, [=]{ return !_count; });
  }

  std::string toString() const {
    std::ostringstream stream;
    stream << "Latch armed with count - " << _count;
    return stream.str();
  }

  private:

  std::mutex _mutex;
  std::condition_variable _cv;
  unsigned _count;
};
