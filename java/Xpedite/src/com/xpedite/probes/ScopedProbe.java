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

    private String buildProbeName(String className, String methodName, boolean isBegin) {
        StringBuffer buffer =new StringBuffer(className);
        buffer.append(".").append(methodName);
        buffer.append(isBegin ? "Begin" : "End");
        return buffer.toString();
    }

    public ScopedProbe(String className, String methodName) {
        super(className, methodName);
        callSites = new CallSite[] {
            new CallSite(this, buildProbeName(className, methodName, true)),
            new CallSite(this, buildProbeName(className, methodName, false))
        };
    }

    public String getClassName() {
        return className;
    }

    public String getMethodName() {
        return methodName;
    }

    public AbstractProbe getBeginProbe() {
        return callSites[0].getProbe();
    }

    public AbstractProbe getEndProbe() {
        return callSites[1].getProbe();
    }

    @Override
    public CallSite[] getCallSites() {
        return callSites;
    }
}
