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

#pragma once

#include <xpedite/util/Errno.H>
#include <xpedite/framework/Persister.H>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <xpedite/transport/Buffer.H>
#include "SamplesFramer.H"
#include <cstdlib> 
#include <regex>
#include <sys/inotify.h>

namespace xpedite { namespace framework {

  class SamplesLoader
  {
    int _fd;
    const FileHeader* _fileHeader;
    CallSiteMap _callSiteMap;
    uint32_t initial_cap = 1024*8;
    xpedite::framework::Framer* _framer;
    const probes::Sample* _sample;
    size_t _fileHeaderSize;
    FileHdrParser* fileParser;
    SegHdrParser* segParser;
    const probes:: Sample* _currentSample;
    static size_t _samplesSize;
    enum MODE _mode;

    public:
    int hasFileHeader;
    
    SamplesLoader()
      : _fd {}, _fileHeader {}, _callSiteMap {}, hasFileHeader {}, _framer {}, _mode {} {
    }

    SamplesLoader(const int fd_, enum MODE mode_)
      : _fd {fd_}, _fileHeader {}, _callSiteMap {}, hasFileHeader {0}, _framer {}, _mode {mode_} {

        fileParser = new FileHdrParser;
        segParser = new SegHdrParser;
        _framer = new Framer(_fd, fileParser, _mode);

        loadFileHeader();
    }

    void loadFileHeader(){
      while(1){
        xpedite::transport::tcp::Frame frame = _framer->readFrame();
        if(frame.isEOF()){ break; }
        if(frame.data() == NULL){ hasFileHeader = 0; }
        else {
          hasFileHeader = 1;
          _framer->resetParser(segParser);
          void* ptr = new char[sizeof(FileHeader)];
          memcpy(ptr, frame.data(), sizeof(FileHeader));
          _fileHeader = reinterpret_cast<const FileHeader*>(ptr);
          if(!_fileHeader->isValid()) { throw std::runtime_error {errorMsg("detected data corruption - mismatch in header signature")}; }
          break;
        }
      }
    }

    xpedite::transport::tcp::Frame loadSegment(){
      xpedite::transport::tcp::Frame frame = _framer->readFrame();
      return frame;
    }

    std::string errorMsg(const char* msg_) {
      xpedite::util::Errno e;
      std::ostringstream os;
      os << msg_ << " - " << e.asString();
      return os.str();
    }

    const CallSiteInfo* locateCallSite(const void* callSite_) const noexcept { return _callSiteMap.locateInfo(callSite_); }
    uint32_t pmcCount()             const noexcept { return _fileHeader->pmcCount(); }
    const CallSiteMap callSiteMap() const noexcept { return _callSiteMap;            }

    uint64_t tscHz() const noexcept {
      if(_fileHeader) {
        return _fileHeader->tscHz();
      }
      return {};
    }

  };

}}

struct file {
  int fd;
  size_t fileSize;
  std::string threadId;
  std::string tlsAddr;
  char* filePath;
  xpedite::framework::SamplesLoader loader;
  int initialInfoPrinted = 0;
};

int segmentSize;
const xpedite::probes::Sample* sample;
std::string filePathS;
std::string fileNameS;
char filePathC[4096];
size_t hasRunId;
int currentFd;
int currentWd;
struct stat fInfo;

void printFileHeader(struct file* currentFile) {
    auto pmcCount = (currentFile->loader).pmcCount();
    std::cout<< currentFile->threadId<<", "<<currentFile->tlsAddr<<", ";
    std::cout << "Tsc,ReturnSite,Data";

    for(unsigned i=0; i<pmcCount; ++i) {
      std::cout << ",Pmc-" << i+1;
    }
    std::cout << std::endl;
    currentFile->initialInfoPrinted = 1;
  }

void printSamples(const xpedite::framework::SegmentHeader* segmentHeader, struct file* currentFile) {
  std::tie(sample, segmentSize) = segmentHeader->samples();
  const void* end = sample + segmentSize/16;

  while(sample < end){
    std::cout<< currentFile->threadId<<", "<<currentFile->tlsAddr<<", ";
    std::cout << std::hex << sample->tsc() << std::dec << "," << sample->returnSite();
    if (sample->hasData()) {
      std::cout << std::hex << "," << std::get<1>(sample->data()) << std::setw(16) << std::setfill('0') 
        << std::right << std::get<0>(sample->data()) << std::dec;
    }
    else { std::cout << ","; }

    if (sample->hasPmc()) {
      const uint64_t* v; int c;
      std::tie(v, c) = sample->pmc();
      for(int i=0; i<c; ++i) {
        std::cout << "," << v[i];
      }
    }
    std::cout << std::endl;
    sample = sample->next();
  }
}

bool newMonitorFile(auto fileNameC, auto dirPathS, auto runId, int fd, struct file* newFile, int* currentWd, enum MODE mode){
  fileNameS = fileNameC;
  hasRunId = fileNameS.find(runId);

  if(hasRunId == std::string::npos){ return false; }

  filePathS = dirPathS + "/" + fileNameS;
  std::strcpy(filePathC, filePathS.c_str());
  currentFd = open(filePathC, O_RDONLY);

  (*currentWd) = inotify_add_watch(fd, filePathC, IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB | IN_OPEN);
  std::regex r("[^\\d]*(\\d+)-(\\d+)-([0-9a-fA-F]+)\\.data");
  std::smatch m;
  std::regex_search(fileNameS, m, r);

  fstat(currentFd, &fInfo);
  xpedite::framework::SamplesLoader currentLoader {currentFd, mode};
  newFile->fd = currentFd;
  newFile->threadId = m[2];
  newFile->tlsAddr = m[3];
  newFile->filePath = filePathC;
  newFile->fileSize = fInfo.st_size;
  newFile->loader = currentLoader;

  return true;
}