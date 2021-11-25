////////////////////////////////////////////////////////////////////////////////////
//
// SamplesLoader loads probe sample data from binary files
//
// Xpedite probes store timing and performance counter data using variable 
// length POD objects. A collection of sample objects is grouped and written
// as a batch. 
//
// The loader iterates through the POD collection,  to extract 
// records in string format for consumption by the profiler
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/SamplesLoader.H>
#include <iostream>

int main(int argc_, char** argv_) {
  if(argc_ <2) {
    std::cerr << "[usage]: " << argv_[0] << " <samples-file>" << std::endl;
    exit(1); 
  }
  xpedite::framework::SamplesLoader::streamAsCsv(argv_[1], std::cout);
  return 0;
}
