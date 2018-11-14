///////////////////////////////////////////////////////////////////////////////////////////////
//
// Stub to enable dynamic loading of shared objects in processess not statically linked to Xpedite
//
// Xpedite expects the binary to statically link to xpedite static library. However the probes 
// can be instrumented in both statically linked and dynamically linked code.
//
// In rare occurences, it might be required to load a shared object with xpedite instrumentation
// in the context of a binary, that's not statically linked to Xpedite.
//
// This file provides a stub implementation to resolve the symbols needed by the shared object.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/probes/ProbeList.H>

xpedite::probes::Trampoline xpediteTrampolinePtr {};

void XPEDITE_CALLBACK xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite) {
}

void XPEDITE_CALLBACK xpediteRemoveProbe(xpedite::probes::Probe*) {
}
