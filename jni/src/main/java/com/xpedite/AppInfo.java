///////////////////////////////////////////////////////////////////////////////
//
// Append new record to xpedite app-info file
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

package com.xpedite;

import com.xpedite.probes.AbstractProbe;
import com.xpedite.probes.AnchoredProbe;
import com.xpedite.probes.CallSite;
import com.xpedite.probes.ScopedProbe;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

class AppInfo {
    private static String toString(CallSite callSite, String attributes) {
        StringBuffer appInfoLine = new StringBuffer("");
        String hexString = "0x" + callSite.getId();
        appInfoLine.append("Id=" + hexString);
        appInfoLine.append(" | Probe=" + hexString);
        appInfoLine.append(" | CallSite=" + hexString);
        appInfoLine.append(" | RecorderReturnSite=" + hexString);
        appInfoLine.append(" | Status=disabled");
        appInfoLine.append(" | Name=" + callSite.getName());
        appInfoLine.append(" | File=" + callSite.getProbe().getFilePath());
        if (callSite.getProbe() instanceof AnchoredProbe) {
            appInfoLine.append(" | Line=" + ((AnchoredProbe) callSite.getProbe()).getLineNo());
        }
        else {
            appInfoLine.append(" | Line=0");
        }
        appInfoLine.append(" | Function=" + callSite.getProbe().getMethodName());
        appInfoLine.append(" | Attributes=" + attributes + "\n");
        return appInfoLine.toString();
    }

    protected static void appendAppInfo(AbstractProbe[] probes) throws IOException {
        PrintWriter out = new PrintWriter(new BufferedWriter(new FileWriter("xpedite-appinfo.txt", true)));
        for (AbstractProbe p: probes) {
            CallSite[] callSites = p.getCallSites();
            if (!(p instanceof ScopedProbe)) {
                String appInfoLine = toString(callSites[0], "None");
                out.append(appInfoLine);
            } else {
                String appInfoLineBegin = toString(callSites[0], "canBeginTxn");
                String appInfoLineEnd = toString(callSites[1], "canEndTxn");
                out.append(appInfoLineBegin);
                out.append(appInfoLineEnd);
            }
        }
        out.close();
    }
}
