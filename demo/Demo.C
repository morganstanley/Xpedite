///////////////////////////////////////////////////////////////////////////////
//
// A stand alone program, demonstrating instrumentation and profiling with Xpedite
// The program can be used to profile latency of random memory acccess vs 
// accessing contiguous memory regions
//
// The program accepts the following command line arguments
//  -m Creates multiple threads for profiling
//  -r Run the random access test
//  -t Transaction count, number of memory accesses done by the program
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include<thread>
#include "Demo.H"

int  cpu {0};
bool multiThreaded {};
int txnCount {100};
bool randomize {};
bool pinMemory {};

void parseArgs(int argc_, char** argv_) {
  int arg;
  while ((arg = getopt (argc_, argv_, "mrt:c:")) != -1) {
    switch (arg) {
    case 'm':
      multiThreaded = true;
      break;
    case 'r':
      randomize = true;
      break;
    case 't':
      txnCount = std::stoi(optarg);
      break;
    case 'c':
      cpu = std::stoi(optarg);
      break;
    case 'l':
      pinMemory = true;
      break;
    case '?':
    default:
      std::cerr << argv_[0] << " [-c <cpu>] [-t <txn count>] [-r] [-m]" << std::endl;
      throw std::invalid_argument{"Invalid argument"};
    }
  }
}

int main(int argc_, char** argv_) {
  using namespace xpedite::demo;
  parseArgs(argc_, argv_);
  std::cout 
    << "\n========================================================================================\n"
    << " \txpedite " << (multiThreaded ? "Multi thread " : "") << "demo [txnCount - " << txnCount 
    << " | randomization - " << (randomize ? "enabled" : "disabled") 
    << " | cpu - " << cpu << "]" 
    << " | pinMemory - " << (pinMemory ? "enabled" : "disabled") << "]" 
    << "\n========================================================================================\n\n";
  initialize(pinMemory);
  if(multiThreaded) {
    int trc;
    std::thread t {[&trc]() {trc = runDemo(txnCount, randomize, cpu);}};
    auto rc = runDemo(txnCount, randomize, cpu);
    t.join();
    return  rc + trc;
  }
  return runDemo(txnCount, randomize, cpu);
}
