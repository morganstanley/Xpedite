# package xpedite.profiler
"""
Module to run profile

This module provides logic to control the duration of the profiling session.
The profiling continues, till one of the following events is detected.
  1. The key press by the user.
  2. Termination of the target application
  3. Expiry of profiling duration

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import sys
import logging

LOGGER = logging.getLogger(__name__)

def profile(app, profileInfo, reportName, reportPath, dryRun, result, # pylint: disable=too-many-locals
  heartbeatInterval=120, interactive=True, duration=None, cprofile=None):
  """
  Orchestrates a Xpedite profile session

  The method starts a profile session by instantiating a xpedite runtime.
  The runtime is kept alive till one of the following conditions are met
  1. The user ends the interactive session by a key press
  2. The total duration for the session elapses
  3. The connection to the profiling target gets closed or disconnected

  Reports and benchmarks are generated at the end of the session

  :param app: An instance of xpedite app, to interact with target application
  :param profileInfo: Parameters and settings for the profile session
  :type profileInfo: xpedite.profileInfo.ProfileInfo
  :param reportName: Name of the profile report
  :type reportName: str
  :param reportPath: Path to persist profile data for benchmarking
  :type reportPath: str
  :param dryRun: Flag to enable simulation of profiling target
  :type dryRun: bool
  :param result: Object for gathering and storing profile results
  :type result: xpedite.jupyter.result.Result
  :param heartbeatInterval: Heartbeat interval for profiler's tcp connection
  :type heartbeatInterval: int
  :param interactive: Flag to enable, an interactive profiling session (Default value = True)
  :type interactive: bool
  :param duration: Profile duration - The session is automatically terminated after elapse
                   of duration seconds (Default value = None)
  :type duration: int
  :param cprofile: Handle to capture self profile Xpedite report generation code (Default value = None)
  :type cprofile: C{xpedite.selfProfile.CProfile}

  """
  import time
  import select
  from xpedite.profiler.runtime import Runtime
  from xpedite.txn.classifier import DefaultClassifier

  runtime = Runtime(
    app=app, probes=profileInfo.probes, pmc=profileInfo.pmc, cpuSet=profileInfo.cpuSet, pollInterval=1,
  )
  if not dryRun:
    begin = time.time()
    elapsed = 0
    duration = int(duration) if duration != None else None

    if interactive:
      LOGGER.info('press RETURN key to, end live profile and generate report ...')

    while True:
      timeout = min(heartbeatInterval, duration - elapsed) if duration != None else heartbeatInterval
      eof = False
      if interactive:
        rlist, _, _ = select.select([sys.stdin], [], [], timeout)
        eof = sys.stdin in rlist
      else:
        time.sleep(timeout)
      elapsed = time.time() - begin
      LOGGER.debug('profile active - elapsed %d / %s seconds | EOF %s', int(elapsed), duration, eof)
      if eof or (duration and elapsed >= duration):
        break
      try:
        app.ping(keepAlive=True)
      except Exception:
        break
  classifier = profileInfo.classifier if profileInfo.classifier else DefaultClassifier()

  if cprofile:
    cprofile.enable()

  runtime.report(result=result, reportName=reportName, benchmarkPaths=profileInfo.benchmarkPaths, classifier=classifier
    , resultOrder=profileInfo.resultOrder, txnFilter=profileInfo.txnFilter)
  if reportPath:
    runtime.makeBenchmark(reportPath)
  return runtime
