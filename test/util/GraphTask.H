///////////////////////////////////////////////////////////////////////////////////////////////
//
// A task spawning graph of child tasks to run in a thread pool
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/framework/Probes.H>
#include "../util/Thread.H"
#include "../util/Latch.H"
#include <mutex>
#include <vector>
#include <atomic>
#include <sstream>

class GraphTask
{
  struct ChildTask
  {
    unsigned _id;
    GraphTask* _root;
    unsigned _level;
    xpedite::framework::TxnId _txnId;
    std::vector<std::unique_ptr<ChildTask>> _children;

    ChildTask(unsigned id_, GraphTask* root_, unsigned level_, xpedite::framework::TxnId txnId_)
      : _id {id_}, _root {root_}, _level {level_}, _txnId {txnId_}, _children {} {
    }

    inline void enque(Thread& thread_, ChildTask* task_) {
      using std::placeholders::_1;
      thread_.enque(std::bind(&ChildTask::runQuantum, task_, _1 ));
    }

    void runQuantum(Thread* thread_) {
      XPEDITE_TXN_RESUME(TaskResume, _txnId);
      if(_level < _root->_depth) {
        log(thread_);
        auto txnId = XPEDITE_TXN_SUSPEND(TaskSuspend);
        for(unsigned i =0; i<_root->_childCount; ++i) {
          auto id = _id*_root->_childCount + i + 1;
          _children.emplace_back(new ChildTask {id, _root, _level+1, txnId});
          enque((*_root->_threads)[id % _root->_threads->size()], _children[i].get());
        }
        return;
      }
      log(thread_);
      XPEDITE_TXN_END(TaskEnd);
      _root->_latch->countDown();
    }

    std::string toString() const {
      std::ostringstream stream;
      stream << "ChildTask level - " << _level << " | id - " << _id;
      return stream.str();
    }

    void log(Thread* thread_) {
      static std::mutex mutex;
      std::lock_guard<std::mutex> lock {mutex};
      std::cout << "Thread [" << thread_->id() << "] executing " << toString() << std::endl;
    }
  };

  friend struct ChildTask;

  public:

  GraphTask(unsigned id_, std::vector<Thread>* threads_, Latch* latch_, unsigned depth_, unsigned childCount_)
    : _id {id_}, _threads {threads_}, _latch {latch_}, _depth {depth_}
    , _child {}, _childCount {childCount_} {
  }

  inline void enque(Thread& thread_) {
    using std::placeholders::_1;
    thread_.enque(std::bind(&GraphTask::run, this, _1));
  }

  void run(Thread* thread_) {
    XPEDITE_TXN_BEGIN(TaskBegin);
    std::cout << "Thread [" << thread_->id() << "] executing root depth - " << _depth << " | child count - " << _childCount << std::endl;
    auto txnId = XPEDITE_TXN_SUSPEND(TaskSuspend);
    _child.reset(new ChildTask {0, this, 0, txnId});
    _child->enque((*_threads)[0], _child.get());
  }

  private:

  unsigned _id;
  std::vector<Thread>* _threads;
  Latch* _latch;
  unsigned _depth;
  std::unique_ptr<ChildTask> _child;
  const unsigned _childCount;
};
