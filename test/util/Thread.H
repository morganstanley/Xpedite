///////////////////////////////////////////////////////////////////////////////////////////////
//
// Thread to await for task from a blocking queue
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <iostream>

class Thread
{
  public:

  using Task = std::function<void(Thread*)>;

  Thread()
    : _canRun {true}, _mutex {}, _cv {}, _queue {}, _thread {&Thread::run, this} {
  }

  void run() {
    Task task;
    while(_canRun) {
      {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [=]{ return !_queue.empty(); });
        task = std::move(_queue.front());
        _queue.pop();
      }
      task(this);
    }
  }

  void enque(Task task_) {
    std::lock_guard<std::mutex> lock {_mutex};
    _queue.emplace(std::move(task_));
    _cv.notify_one();
  }

  void join() {
    _canRun = {};
    enque([](Thread* thread_){std::cout << "Awaiting thread [" << thread_->id() << "] termination ..." << std::endl;});
    _thread.join();
  }

  std::thread::id id() {
    return _thread.get_id();
  }

  private:

  bool _canRun;
  std::mutex _mutex;
  std::condition_variable _cv;
  std::queue<Task> _queue;
  std::thread _thread;
};

