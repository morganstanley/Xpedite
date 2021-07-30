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
#include <iomanip>
#include <ios>

int main(int argc_, char** argv_) {
  if(argc_ <2) {
    std::cerr << "[usage]: " << argv_[0] << " <samples-file>" << std::endl;
    exit(1); 
  }

  using namespace xpedite::probes;
  using namespace xpedite::framework;
  SamplesLoader loader {argv_[1]};
  auto pmcCount = loader.pmcCount();
  std::cout << "Tsc,ReturnSite,Data";
  for(unsigned i=0; i<pmcCount; ++i) {
    std::cout << ",Pmc-" << i+1;
  }
  std::cout << std::endl;

  for(auto& sample : loader) {
    std::cout << std::hex << sample.tsc() << std::dec << "," << sample.returnSite();
    if (sample.hasData()) {
      std::cout << std::hex << "," << std::get<1>(sample.data()) << std::setw(16) << std::setfill('0') 
        << std::right << std::get<0>(sample.data()) << std::dec;
    }
    else {
      std::cout << ",";
    }

    if (sample.hasPmc()) {
      const uint64_t* v; int c;
      std::tie(v, c) = sample.pmc();
      for(int i=0; i<c; ++i) {
        std::cout << "," << v[i];
      }
    }
    std::cout << std::endl;
  }
  return 0;
}
