///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target app to test memory allocation intercept functionality
//
// This app allocates memory using a variety of methods in each transaction
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>
#include "../util/Args.H"
#include <stdexcept>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc_, char** argv_) {

  using namespace xpedite::framework;
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", {AWAIT_PROFILE_BEGIN})) { 
    throw std::runtime_error {"failed to init xpedite"}; 
  }

  auto args = parseArgs(argc_, argv_);

  using Type = int;
  using Pointer = int*;
  constexpr int ALIGNMENT = 2048;

  Pointer ptr;
  for(int i=0; i<args.txnCount; ++i) {
    XPEDITE_TXN_SCOPE(Allocation);

    ptr = new Type {};
    delete ptr;

    ptr = new Type[4] {};
    delete[] ptr;

    if((ptr = static_cast<Pointer>(malloc(sizeof(Type))))) {
      free(ptr);
    }

    if((ptr = static_cast<Pointer>(calloc(1, sizeof(Type))))) {
      if((ptr = static_cast<Pointer>(realloc(ptr, 2*sizeof(Type))))) {
        free(ptr);
      }
    }

    if(!posix_memalign(reinterpret_cast<void**>(&ptr), ALIGNMENT, sizeof(Type))) {
      free(ptr);
    }

    auto size = getpagesize();
    if((ptr = static_cast<Pointer>(mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))) != MAP_FAILED) {
      munmap(ptr, size);
    }
  }
  return 0;
}
