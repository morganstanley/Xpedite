///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target app to generate txn chained across multiple threads
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include "../util/Task.H"
#include "../util/GraphTask.H"
#include "../util/Thread.H"
#include "../util/Latch.H"
#include <unistd.h>
#include <vector>
#include <memory>
#include <cmath>

unsigned childCount {2};
unsigned threadCount {4};
unsigned txnCount {100};
unsigned cpu {0};

void parseArgs(int argc_, char** argv_) {
  int arg;
  while ((arg = getopt (argc_, argv_, "m:t:g:c:")) != -1) {
    switch (arg) {
    case 'g':
      childCount = std::stoi(optarg);
      break;
    case 'm':
      threadCount = std::stoi(optarg);
      break;
    case 't':
      txnCount = std::stoi(optarg);
      break;
    case 'c':
      cpu = std::stoi(optarg);
      break;
    case '?':
    default:
      std::cerr << argv_[0] << " [-T <thread count>] [-t <txn count>] [-g graphChildCount]" << std::endl;
      throw std::invalid_argument{"Invalid argument"};
    }
  }
}

template<typename Task, typename... Args>
void runTest(unsigned txnCount_, unsigned latchCount_, Args&&... args_) {
  Latch latch {latchCount_};
  std::vector<Thread> threads {threadCount};
  std::vector<std::unique_ptr<Task>> tasks;
  std::cout << "Run test with " << threadCount << " thread(s) | " << txnCount_ << " transaction(s) | " << latch.toString() << std::endl;
  for(unsigned i=0; i<txnCount_; ++i) {
    tasks.emplace_back(new Task {i, &threads, &latch, std::forward<Args>(args_)...});
  }

  for(auto& task: tasks) {
    task->enque(threads[0]);
  }
  latch.wait();

  for(auto& thread: threads) {
    thread.join();
  }
}

int main(int argc_, char** argv_) {

  parseArgs(argc_, argv_);

  using namespace xpedite::framework;
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", {AWAIT_PROFILE_BEGIN})) { 
    throw std::runtime_error {"failed to init xpedite"}; 
  }
  if(childCount) {
    unsigned depth = std::ceil(std::log(txnCount) / std::log(childCount));
    auto latchCount = static_cast<unsigned>(std::pow(childCount, depth));
    runTest<GraphTask>(1, latchCount, depth, childCount);
  }
  else {
    auto timeToLive = 8;
    runTest<Task>(txnCount, txnCount, timeToLive);
  }
  return 0;
}
