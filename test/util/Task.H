///////////////////////////////////////////////////////////////////////////////////////////////
//
// A task running quantums of work across a pool of threads to completion
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

class Task
{
  public:

  Task(unsigned id_, std::vector<Thread>* threads_, Latch* latch_, int timeToLive_)
    : _id {id_}, _threads {threads_}, _latch {latch_}, _timeToLive {timeToLive_}, _quantum {}, _txnId {} {
  }

  inline void enque(Thread& thread_) {
    using std::placeholders::_1;
    thread_.enque(std::bind(&Task::runQuantum, this, _1 ));
  }

  void runQuantum(Thread* thread_) {
    if(_quantum == 0) {
      XPEDITE_TXN_BEGIN(TaskBegin);
    }
    else {
      XPEDITE_TXN_RESUME(TaskResume, _txnId);
    }
    if(++_quantum <= _timeToLive) {
      log(thread_);
      _txnId = XPEDITE_TXN_SUSPEND(TaskSuspend);
      enque((*_threads)[_quantum % _threads->size()]);
      return;
    }
    log(thread_);
    XPEDITE_TXN_END(TaskEnd);
    _latch->countDown();
  }

  protected:

  void log(Thread* thread_) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock {mutex};
    std::cout << "Thread [" << thread_->id() << "] executing task " << _id << " | quantum " << _quantum << std::endl;
  }

  unsigned _id;
  std::vector<Thread>* _threads;
  Latch* _latch;
  int _timeToLive;
  int _quantum;
  xpedite::framework::TxnId _txnId;
};
