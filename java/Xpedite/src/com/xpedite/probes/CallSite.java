///////////////////////////////////////////////////////////////////////////////
//
// Representation of a call site
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

package com.xpedite.probes;

public class CallSite {
    private AbstractProbe probe;
    private int id;
    protected String name;
    protected static int callSiteCount = 1;

    public CallSite(AbstractProbe probe, String name) {
        this.probe = probe;
        this.name = name;
        this.id = callSiteCount;
        ++callSiteCount;
    }

    public AbstractProbe getProbe() {
        return probe;
    }

    public int getId() {
        return id;
    }

    public String getName() {
        return name;
    }

}