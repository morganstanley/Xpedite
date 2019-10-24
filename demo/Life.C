///////////////////////////////////////////////////////////////////////////////
//
// A stand alone program, demonstrating instrumentation and profiling with Xpedite
// The program provides a trivial example for usage of transaction and probe API.
//
// A binary (named life) can be built by invoking the following command
// after substitution of xpedite install directories
//    g++ -pthread -std=c++11 -I <path-to-xpedite-headers> Life.C -o life -L <path-to-xpedite-libs> -lxpedite -ldl -lrt
//
// To generate a profile info file, use geneate command with xpedite, after launching binary
//    xpedite generate -a /tmp/xpedite-appinfo.txt
// 
// To create a profile report, use record command with xpedite, while the binary is running
//    xpedite record -p profileInfo.py
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <stdexcept>
#include <iostream>
#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>
#include <xpedite/framework/Options.H>

void eat()   { std::cout << "eat..."   << std::endl; }
void sleep() { std::cout << "sleep..." << std::endl; }
void code()  { std::cout << "code..."  << std::endl; }

void life(int timeToLive_) {
  for(unsigned i=0; i<timeToLive_; ++i) {
    XPEDITE_TXN_SCOPE(Life);
    eat();

    XPEDITE_PROBE(SleepBegin);
    sleep();

    XPEDITE_PROBE(CodeBegin);
    code();
  }
}

int main() {
  const xpedite::framework::Options options = {xpedite::framework::AWAIT_PROFILE_BEGIN};
  if(!xpedite::framework::initialize("/tmp/xpedite-appinfo.txt", options)) { 
    throw std::runtime_error {"failed to init xpedite"}; 
  }
  life(100);
}
