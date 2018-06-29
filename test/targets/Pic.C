///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target to test probes in position independent code
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <xpedite/framework/Probes.H>

void foo() {
  XPEDITE_PROBE_SCOPE(Foo);
  std::cout << "Foo ..." << std::endl;
}

void bar() {
  XPEDITE_PROBE_SCOPE(Bar);
  std::cout << "Bar ..." << std::endl;
}

void positionIndependentCode() {
  foo();
  bar();
}
