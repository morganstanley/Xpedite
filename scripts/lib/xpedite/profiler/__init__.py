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
import os
import logging
import xpedite
from xpedite.profiler.profileInfo       import loadProfileInfo
from xpedite.profiler.app               import XpediteApp, XpediteDormantApp, pingApp
from logger                             import enableVerboseLogging
from xpedite.dependencies               import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Six)

LOGGER = logging.getLogger(__name__)

def buildReportName(appName, reportName):
  """Constructs report name from app + user supplied report name"""
  import re
  from datetime import datetime
  reportName = reportName if reportName else '{}-{}'.format(appName, datetime.now().strftime('%Y-%m-%d-%H:%M:%S'))
  reportName = re.sub(r'[^\w\-:_\. ]', '_', reportName)
  return reportName

def validateBenchmarkPath(path):
  """Validate the given path for write access"""
  if path:
    if os.path.exists(path):
      LOGGER.error('cannot create/overwrite benchmark at "%s". path already exists\n', path)
      sys.exit(10)
    xpedite.util.mkdir(path)

def _loadProbes(app):
  """Attaches to application and loads probe data"""
  from xpedite.profiler.probeAdmin import ProbeAdmin
  try:
    with app:
      pingApp(app)
      return ProbeAdmin.loadProbes(app)
  except Exception as _:
    return list(app.appInfo.probes.values())

class Profiler(object):
  """Xpedite Profiler"""

  @staticmethod
  def profile(app, profileInfo, reportName, reportPath, dryRun, # pylint: disable=too-many-locals
    heartbeatInterval=120, samplesFileSize=None, interactive=True, duration=None, cprofile=None):
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
    :param heartbeatInterval: Heartbeat interval for profiler's tcp connection
    :type heartbeatInterval: int
    :param samplesFileSize: Max size of data files used to store samples
    :type samplesFileSize: int
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
      app=app, probes=profileInfo.probes, pmc=profileInfo.pmc, cpuSet=profileInfo.cpuSet,
      pollInterval=1, samplesFileSize=samplesFileSize,
    )
    if not dryRun:
      begin = time.time()
      elapsed = 0
      duration = int(duration) if duration is not None else None

      if interactive:
        LOGGER.info('press RETURN key to, end live profile and generate report ...')

      while True:
        timeout = min(heartbeatInterval, duration - elapsed) if duration is not None else heartbeatInterval
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

    report = runtime.report(reportName=reportName, benchmarkPaths=profileInfo.benchmarkPaths
        , classifier=classifier, resultOrder=profileInfo.resultOrder, txnFilter=profileInfo.txnFilter
        , routeConflation=profileInfo.routeConflation)
    if reportPath:
      report.makeBenchmark(reportPath)
    return report

  @staticmethod
  def record(profileInfoPath, benchmarkPath=None, duration=None, heartbeatInterval=None,
      samplesFileSize=None, cprofile=None, profileName=None, verbose=None):
    """
    Records an xpedite profile using the supplied parameters

    :param profileInfoPath: Path to profile info module
    :type profileInfoPath: str
    :param benchmarkPath: Path to persist profile data for benchmarking
    :type benchmarkPath: str
    :param duration: Profile duration - The session is automatically terminated after elapse
                     of duration seconds (Default value = None)
    :type duration: int
    :param heartbeatInterval: Heartbeat interval for profiler's tcp connection
    :type heartbeatInterval: int
    :param samplesFileSize: Max size of data files used to store samples
    :type samplesFileSize: int
    :param cprofile: Handle to capture self profile Xpedite report generation code (Default value = None)
    :type cprofile: C{xpedite.selfProfile.CProfile}
    :param profileName: Name of the profile report
    :type profileName: str
    :param verbose: Flag to enable, verbose logging
    :type verbose: bool
    """
    if verbose:
      enableVerboseLogging()
    profileInfo = loadProfileInfo(profileInfoPath)
    validateBenchmarkPath(benchmarkPath)
    app = XpediteApp(profileInfo.appName, profileInfo.appHost, profileInfo.appInfo)
    with app:
      reportName = buildReportName(profileInfo.appName, profileName)
      report = Profiler.profile(
        app, profileInfo, reportName, benchmarkPath, False, heartbeatInterval=heartbeatInterval,
        samplesFileSize=samplesFileSize, duration=duration, cprofile=cprofile
      )
    return profileInfo, report

  @staticmethod
  def report(profileInfoPath, runId=None, dataSourcePath=None, benchmarkPath=None, cprofile=None,
      profileName=None, verbose=None):
    """
    Generates report for a previous profiling runs

    :param profileInfoPath: Path to profile info module
    :type profileInfoPath: str
    :param runId: Unique identifier for a previous run
    :type runId: str
    :param dataSourcePath: Path to load txn data for reporting
    :param benchmarkPath: Path to persist profile data for benchmarking
    :type benchmarkPath: str
    :param cprofile: Handle to capture self profile Xpedite report generation code (Default value = None)
    :type cprofile: C{xpedite.selfProfile.CProfile}
    :param profileName: Name of the profile report
    :type profileName: str
    :param verbose: Flag to enable, verbose logging
    :type verbose: bool
    """
    if verbose:
      enableVerboseLogging()
    profileInfo = loadProfileInfo(profileInfoPath)
    validateBenchmarkPath(benchmarkPath)
    app = XpediteDormantApp(profileInfo.appName, profileInfo.appHost, profileInfo.appInfo,
        runId=runId, dataSourcePath=dataSourcePath)
    with app:
      reportName = buildReportName(profileInfo.appName, profileName)
      report = Profiler.profile(app, profileInfo, reportName, benchmarkPath, True, cprofile=cprofile)
    return profileInfo, report

  @staticmethod
  def probes(profileInfoPath):
    """
    Attaches to application and loads probe data

    :param profileInfoPath: Path to profile info module
    :type profileInfoPath: str
    """
    profileInfo = loadProfileInfo(profileInfoPath)
    app = XpediteApp('app', profileInfo.appHost, profileInfo.appInfo)
    return _loadProbes(app), profileInfo

  @staticmethod
  def generate(appInfoPath, hostname=None):
    """
    Attaches to application and generates default profile info

    :param appInfoPath: Path to app info module
    :type appInfoPath: str
    :param hostname: Name of the host running the target process
    :type hostname: str
    """
    hostname = hostname if hostname else 'localhost'
    app = XpediteApp('app', hostname, appInfoPath)
    probes = _loadProbes(app)
    if probes:
      from xpedite.profiler.profileInfoGenerator import ProfileInfoGenerator
      appInfoAbsolutePath = os.path.abspath(appInfoPath)
      profilerPath = os.path.abspath(os.path.join(__file__, '../../../../bin/xpedite'))
      return ProfileInfoGenerator(app.executableName, hostname, appInfoAbsolutePath, probes, profilerPath).generate()
    LOGGER.error('failed to generate profile_info.py. cannot locate probes in app. Have you instrumented any ?\n')
    return None
