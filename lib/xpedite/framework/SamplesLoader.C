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
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <ios>

namespace xpedite { namespace framework {

  int SamplesLoader::saveAsCsv(const char* samplesPath_, const char* dest_) {
    using namespace xpedite::probes;
    using namespace xpedite::framework;

    std::ofstream destStream;
    try {
      destStream.open(dest_, std::ios_base::out);
    }
    catch(std::ios_base::failure& e) {
      std::ostringstream stream;
      stream << "xpedite failed to open log " << dest_ << " for writing - " << e.what();
      throw std::runtime_error {stream.str()};
    }
    return streamAsCsv(samplesPath_, destStream);
  }

  int SamplesLoader::streamAsCsv(const char* samplesPath_, std::ostream& destStream_) {
    using namespace xpedite::probes;
    using namespace xpedite::framework;

    SamplesLoader loader {samplesPath_};
    auto pmcCount = loader.pmcCount();
    destStream_ << "Tsc,ReturnSite,Data";
    for(unsigned i=0; i<pmcCount; ++i) {
      destStream_ << ",Pmc-" << i+1;
    }
    destStream_ << std::endl;

    int count {};
    for(auto& sample : loader) {
      destStream_ << std::hex << sample.tsc() << std::dec << "," << sample.returnSite();
      if (sample.hasData()) {
        destStream_ << std::hex << "," << std::get<1>(sample.data()) << std::setw(16) << std::setfill('0') 
          << std::right << std::get<0>(sample.data()) << std::dec;
      }
      else {
        destStream_ << ",";
      }

      if (sample.hasPmc()) {
        const uint64_t* v; int c;
        std::tie(v, c) = sample.pmc();
        for(int i=0; i<c; ++i) {
          destStream_ << "," << v[i];
        }
      }
      destStream_ << std::endl;
      ++count;
    }
    return count;
  }

}}
