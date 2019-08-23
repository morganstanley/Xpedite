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

#include <iostream>
#include <iomanip>
#include <ios>
#include <stdio.h>
#include <map>
#include <stdexcept>
#include <xpedite/util/Errno.H>
#include <string.h>
#include <sys/inotify.h>
#include "SamplesLoader.H"
#include <unistd.h>
#include <dirent.h>

std::string errorMsg(const char* msg_) {
      xpedite::util::Errno e;
      std::ostringstream os;
      os << msg_ << " - " << e.asString();
      return os.str();
    }
    

int main(int argc_, char** argv_) {

  if(argc_ <2) {
    std::cerr << "[usage]: " << argv_[0] << " <samples-file>" << std::endl;
    exit(1); 
  }

  using namespace xpedite::probes;
  using namespace xpedite::framework;  

  int fd;
  int length;
  int buffLen = 128 * 1024 * sizeof(struct inotify_event);
  char buff[buffLen];
  int currentFd;
  int currentWd;
  int dirWd;
  char dirPathC[255] = argv_[3];
  static char* fileNameC;
  static std::string dirPathS = dirPathC;
  std::string threadInfo[2];
  std::map<int, struct file> wd_file;
  struct file currentFile;
  const SegmentHeader* _segmentHeader;
  static char* runId = argv_[1];
  std::string fileNameS;
  enum MODE mode;

  if(strcmp(argv_[2], "BATCH") == 0)
    mode = BATCH;
  else
    mode = REALTIME;

  memset(buff, 0, sizeof(buff));

  fd = inotify_init();
  dirWd = inotify_add_watch(fd, dirPathC, IN_CREATE);

  DIR *dir;
  struct dirent *entry;
  dir = opendir(dirPathC);
  if(dir == NULL){ throw std::runtime_error {errorMsg("invalid directory")}; }

  while(entry = readdir(dir)){
    fileNameC = entry->d_name;
    struct file newFile;
    if(!newMonitorFile(fileNameC, dirPathS, runId, fd, &newFile, &currentWd, mode)){
      continue;
    }

    while(1){
      if(!newFile.loader.hasFileHeader){
        newFile.loader.loadFileHeader();
      }
      if(!newFile.loader.hasFileHeader){
        break;
      }
      if(newFile.loader.hasFileHeader && !(newFile.initialInfoPrinted)){
        printFileHeader(&newFile);
      }

      xpedite::transport::tcp::Frame frame = newFile.loader.loadSegment();
      if(frame.data() == NULL){
        if(frame.isEOF()){
          break;
        }
        continue;
      }

      _segmentHeader = reinterpret_cast<const SegmentHeader*>(frame.data());
      if(!_segmentHeader->isValid()){ throw std::runtime_error {errorMsg("invalid segment header2")}; };
      printSamples(_segmentHeader, &newFile);
    }
    wd_file.emplace(currentWd, newFile);
  }
  closedir(dir);

  while(mode == REALTIME){
    struct inotify_event *current;
    struct inotify_event *end;
    length = read(fd, buff, buffLen);

    if(length < 0)
      throw std::runtime_error {errorMsg("error for file events in inotify")};
    else{
      current = (struct inotify_event*) &buff[0];
      end = (struct inotify_event*) &buff[length];

      while(current < end){
        if(current->wd == dirWd){
          if(current->mask & IN_CREATE){
            if(!(current->mask & IN_ISDIR)){

              fileNameC = current->name;
              struct file newFile;
              if(!newMonitorFile(fileNameC, dirPathS, runId, fd, &newFile, &currentWd, mode)){
                continue;
              }
              wd_file.emplace(currentWd, newFile);

              if(newFile.loader.hasFileHeader && !(newFile.initialInfoPrinted)){
                printFileHeader(&newFile);
              }
            }
            else{ throw std::runtime_error {errorMsg("error for monitoring directory")}; }
          }
          else{ throw std::runtime_error {errorMsg("error for monitoring directory")}; }
          current = (inotify_event *) ((char*)current + sizeof(*current) + current->len);
        }
        else{
          currentFile = wd_file[current->wd];
          currentFd = currentFile.fd;

          while(1){
            if(!currentFile.loader.hasFileHeader){
              currentFile.loader.loadFileHeader();
            }
            if(!currentFile.loader.hasFileHeader){
              break;
            }
            if(currentFile.loader.hasFileHeader && !(currentFile.initialInfoPrinted)){
              printFileHeader(&currentFile);
            }

            xpedite::transport::tcp::Frame frame = currentFile.loader.loadSegment();
            if(frame.data() == NULL){
              if(frame.isEOF())
                break;
              continue;
            }

            _segmentHeader = reinterpret_cast<const SegmentHeader*>(frame.data());
            if(!_segmentHeader->isValid()){ throw std::runtime_error {errorMsg("invalid segment header2")}; };
            printSamples(_segmentHeader, &currentFile);
          }

          wd_file[current->wd] = currentFile;
          current = (inotify_event *)((char*)current + sizeof(*current));
        }
      }
    }
  }

  return 0;
}