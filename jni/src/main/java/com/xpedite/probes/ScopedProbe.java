///////////////////////////////////////////////////////////////////////////////
//
// Java implementation of Xpedite scoped probes
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

package com.xpedite.probes;

public class ScopedProbe extends AbstractProbe {

    private CallSite[] callSites;

    public ScopedProbe(String className, String methodName) {
        super(className, methodName);
        callSites = new CallSite[] {
            new CallSite(className, methodName, 0),
            new CallSite(className, methodName, Integer.MAX_VALUE)
        };
    }

    public String getClassName() {
        return className;
    }

    public String getMethodName() {
        return methodName;
    }

    @Override
    public CallSite[] getCallSites() {
        return callSites;
    }
}
