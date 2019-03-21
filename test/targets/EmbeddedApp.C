///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target app to test embedded profiling from process context
//
// This app intializes framework and profiling session standalone to collect profile data.
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

void foo() {
  XPEDITE_PROBE_SCOPE(Foo);
}

void bar() {
  XPEDITE_PROBE_SCOPE(Bar);
}

void baz() {
  XPEDITE_PROBE_SCOPE(Baz);
}

int main(int argc_, char** argv_) {
  auto args = parseArgs(argc_, argv_);

  using namespace xpedite::framework;
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", {DISABLE_REMOTE_PROFILING})) { 
    throw std::runtime_error {"failed to init xpedite"}; 
  }

  xpedite::framework::ProfileInfo profileInfo {
    {"TxnBegin", "TxnEnd", "FooBegin", "FooEnd"},
    PMUCtlRequest {
      ._cpu = 0, ._fixedEvtCount = 1, ._gpEvtCount = 0, ._offcoreEvtCount = 0,
      ._fixedEvents = {
        PMUFixedEvent {._ctrIndex = 0, ._user = 1, ._kernel = 1}
      },
      ._gpEvents = {},
      ._offcoreEvents = {}
    },
    400000
  };

  auto guard = xpedite::framework::profile(profileInfo);

  std::cout << "Begin profile" << std::endl;
  for(int i=0; i<args.txnCount; ++i) {
    XPEDITE_TXN_SCOPE(Txn);
    foo();
    bar();
    baz();
    if(i % 100 == 0) {
      usleep(100000);
    }
  }
  std::cout << "End profile" << std::endl;
  return 0;
}
