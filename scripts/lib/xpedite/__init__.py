"""
Xpedite package

Xpedite is a probe based profiler to measure and optimise,
performance of ultra-low-latency / real time systems.

The main features include

  1. Quantify how efficiently "a software stack" or
     "a section of code", is running in a target platform (CPU/OS).
  2. Do Cycle accounting and bottleneck analysis using H/W performance
     counters and top-down micro architecture analysis methodology
  3. Filter, query and visualise performance statistics with
     real time interactive shell (Jupiter).
  4. Prevent regressions, by benchmarking latency statistics
     for multiple runs/builds side-by-side.

Author: Manikandan Dhamodharan, Morgan Stanley
"""
from xpedite.dependencies     import Package, DEPENDENCY_LOADER
from xpedite.probe            import Probe, TxnBeginProbe, TxnSuspendProbe, TxnResumeProbe, TxnEndProbe
from xpedite.types            import ResultOrder
from xpedite.pmu.event        import Event, TopdownNode, Metric
