///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target to test txn builing in app linking to position independent code
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>
#include "../util/Args.H"
#include <stdexcept>

void positionIndependentCode();

int main(int argc_, char** argv_) {

  auto args = parseArgs(argc_, argv_);

  using namespace xpedite::framework;
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", {AWAIT_PROFILE_BEGIN})) { 
    throw std::runtime_error {"failed to init xpedite"}; 
  }

  for(int i=0; i<args.txnCount; ++i)
  {
    XPEDITE_TXN_SCOPE(Txn);
    positionIndependentCode();
  }
}
