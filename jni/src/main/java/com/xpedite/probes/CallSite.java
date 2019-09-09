///////////////////////////////////////////////////////////////////////////////
//
// Representation of a call site
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

package com.xpedite.probes;

public class CallSite {
    private int id;
    private String name;
    private String className;
    private String methodName;
    private int lineNo;
    private static int callSiteCount = 1;

    public CallSite(String className, String methodName, int lineNo) {
        this.className = className;
        this.methodName = methodName;
        this.lineNo = lineNo;
        this.id = callSiteCount++;
        this.name = buildProbeName();
    }

    public String getClassName() {
	    return className;
    }

    public String getMethodName() {
	    return methodName;
    }

    public int getLineNo() {
        return lineNo;
    }

    public int getId() {
        return id;
    }

    public String getName() {
        return name;
    }

    private String buildProbeName() {
        StringBuffer buffer = new StringBuffer(className);
        buffer.append(".").append(methodName);
        if (lineNo == 0) {
            buffer.append("Begin");
        } else if (lineNo == Integer.MAX_VALUE) {
            buffer.append("End");
        } else {
            buffer.append(lineNo);
        }
        return buffer.toString();
    }
}
