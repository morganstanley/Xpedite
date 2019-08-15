///////////////////////////////////////////////////////////////////////////////
//
// Java implementation of Xpedite probes with line numbers
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

package com.xpedite.probes;

public class AnchoredProbe extends AbstractProbe {

    private int lineNo;
    private CallSite[] callSites;

    private String buildProbeName(String className, String methodName, int lineNo) {
        StringBuffer buffer = new StringBuffer(className);
        buffer.append(".").append(methodName);
        buffer.append(":").append(lineNo);
        return buffer.toString();
    }

    public AnchoredProbe(String className, String methodName, int lineNo) {
        super(className, methodName);
        callSites = new CallSite[] { new CallSite(this, buildProbeName(className, methodName, lineNo))};
    }

    public int getLineNo() {
        return this.lineNo;
    }

    @Override
    public CallSite[] getCallSites() {
        return callSites;
    }
}
