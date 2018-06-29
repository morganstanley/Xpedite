///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target to test txn builing in app linking to position independent code
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>
#include <stdexcept>

void positionIndependentCode();

int main() {
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", true)) { 
    throw std::runtime_error {"failed to init xpedite"}; 
  }

  for(int i=0; i<100; ++i)
  {
    XPEDITE_TXN_SCOPE(Txn);
    positionIndependentCode();
  }
}
