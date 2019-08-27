package com.xpedite.demo;
///////////////////////////////////////////////////////////////////////////////
//
//Functions for testing Xpedite profiling
//
//Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

public class App {

  public void run() {
      for (int i = 1; i <= 50; i++) {
          doCompute();
          doIo(i);
      }
  }

  public void doCompute() {
      long begin = System.nanoTime();
      while(true) {
          // spin for 10 us
          long end = System.nanoTime();
          if(end - begin >= 10000) {
              break;
          }
      }
  }

  public void doIo(int iteration_) {
      System.out.println("[Java Application] Iteration: " + iteration_);
  }
}
