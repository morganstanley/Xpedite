///////////////////////////////////////////////////////////////////////////////
//
// Abstract Java implementation of Xpedite probes
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

package com.xpedite.probes;

public abstract class AbstractProbe {

    protected String className;
    protected String methodName;
    protected String filePath;

    public AbstractProbe(String className, String methodName) {
        this.className = className;
        this.methodName = methodName;
        this.filePath = className + ".java";
    }

    public String getClassName() {
        return className;
    }

    public String getMethodName() {
        return methodName;
    }

    public String getFilePath() {
        return filePath;
    }

    abstract public CallSite[] getCallSites();
}
