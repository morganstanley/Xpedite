package com.xpedite.demo;
///////////////////////////////////////////////////////////////////////////////
//
//Application for testing embedded profiling with Xpedite
//
//Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////


import com.xpedite.Xpedite;
import com.xpedite.probes.AbstractProbe;
import com.xpedite.probes.ScopedProbe;
import com.xpedite.probes.AnchoredProbe;

public class EmbeddedDemo {

    public static void main(String args[]) {
      String appClass = "com/xpedite/demo/App";
      try {
        AbstractProbe[] probes = new AbstractProbe[] {
          new AnchoredProbe(appClass, "doCompute", 12),
          new ScopedProbe(appClass, "doCompute"),
          new ScopedProbe(appClass, "doIo")
        };
        com.xpedite.Xpedite.getInstance().profile(probes);
        new App().run();
      }
      catch(Exception e) {
        e.printStackTrace();
      }
    }
}
