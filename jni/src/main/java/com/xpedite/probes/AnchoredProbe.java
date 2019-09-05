///////////////////////////////////////////////////////////////////////////////
//
// Java implementation of Xpedite probes with line numbers
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

package com.xpedite.probes;

public class AnchoredProbe extends AbstractProbe {

    private CallSite[] callSites;

    public AnchoredProbe(String className, String methodName, int lineNo) {
        super(className, methodName);
        callSites = new CallSite[] { new CallSite(className, methodName, lineNo)};
    }

    @Override
    public CallSite[] getCallSites() {
        return callSites;
    }
}
