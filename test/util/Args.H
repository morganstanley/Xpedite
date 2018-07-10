///////////////////////////////////////////////////////////////////////////////////////////////
//
// Utility file to parse common arguments for different target applications
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <cstdlib>
#include <unistd.h>

struct Args
{
  int threadCount {4};
  int txnCount {100};
  int cpu {0};
};

inline Args parseArgs(int argc_, char** argv_) {
  int opt;
  Args args;
  while ((opt = getopt (argc_, argv_, "T:t:c:")) != -1) {
    switch (opt) {
    case 'T':
      args.threadCount = std::stoi(optarg);
      break;
    case 't':
      args.txnCount = std::stoi(optarg);
      break;
    case 'c':
      args.cpu = std::stoi(optarg);
      break;
    case '?':
    default:
      std::cerr << argv_[0] << " [-T <thread count>] [-t <txn count>]" << std::endl;
      exit(1);
    }
  }
  return args;
}
