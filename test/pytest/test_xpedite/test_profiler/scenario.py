# pylint: disable=wrong-import-position
"""
This module contains different scenarios that can be tested by Xpedite pytests,
and loads files that are used as parameters and the expected results for
comparison in the tests. Scenarios are loaded for targets applications including:
1. An application that allocates memory
2. An application to exercise txn carrying different units of data
3. A multithreaded application
4. An application to parse FIX messages

Each application is used in 2 scenarios:
1. A regular scenario
2. A scenario using Xpedite's benchmarks

Author: Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
import json
import time
import logging
import logging.config
from xpedite.dependencies               import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Enum)
DEPENDENCY_LOADER.load(Package.Rpyc)
from enum                               import Enum
import rpyc
from test_xpedite                       import (
                                          REPORT_CMD_BASELINE_PATH,
                                          PROFILE_INFO_PATH,
                                          DATA_FILE_EXT, BASELINE_CPU_INFO_PATH,
                                          PROBE_CMD_BASELINE_PATH,
                                          GENERATE_CMD_BASELINE_PATH,
                                          XPEDITE_APP_INFO_PATH,
                                          XPEDITE_APP_INFO_PARAMETER_PATH,
                                          PARAMETERS_DATA_DIR, DIR_PATH, SRC_DIR_PATH,
                                          LOCALHOST, BINARY_PATH, loadProfileInfo
                                        )
from test_xpedite.test_profiler.profile import validateBenchmarks
from xpedite.jupyter                    import PROFILES_KEY
from logger                             import LOG_CONFIG_PATH

logging.config.fileConfig(LOG_CONFIG_PATH)
LOGGER = logging.getLogger('xpedite')

class ScenarioType(Enum):
  """Scenarios used in testing"""
  Benchmark = 'Benchmark'
  Regular = 'Regular'
  PMC = 'PMC'

  def __str__(self):
    """
    String representation of scenario types
    """
    return self.value

class ParameterFiles(object):
  """
  Load and store parameters for a scenario
  """
  def __init__(self, dataDir, tempDir, remote=None):
    """
    Load input files for a scenario
    """
    from xpedite.util.cpuInfo import decodeCpuInfo
    with open(os.path.join(dataDir, BASELINE_CPU_INFO_PATH)) as fileHandle:
      self.fullCpuInfo = json.load(fileHandle, object_hook=decodeCpuInfo)
    appInfoPath = os.path.join(tempDir, XPEDITE_APP_INFO_PATH)
    self.profileInfo = loadProfileInfo(
      dataDir, PROFILE_INFO_PATH, appInfoPath=appInfoPath, remote=remote
    )

class ExpectedResultFiles(object):
  """
  Set expected results for a scenario
  """
  def __init__(self, dataDir):
    """
    Load files with expected results for comparison
    """
    from six.moves import cPickle as pickle
    from xpedite.jupyter.xpediteData    import XpediteDataReader
    with open(os.path.join(dataDir, PROBE_CMD_BASELINE_PATH), 'rb') as probeFileHandle:
      self.baselineProbeMap = pickle.load(probeFileHandle) # pylint: disable=c-extension-no-member
    with XpediteDataReader(os.path.join(dataDir, REPORT_CMD_BASELINE_PATH)) as xpediteDataReader:
      self.baselineProfiles = xpediteDataReader.getData(PROFILES_KEY)
    self.baselineProfileInfo = loadProfileInfo(dataDir, GENERATE_CMD_BASELINE_PATH)

class Scenario(object):
  """
  Load parameters and expected results information for a specific scenario of a test
  """
  def __init__(self, runPath, appName, name, scenarioType, remote=None):
    """
    Create a scenario and load parameters and expected results
    """
    self.tempDir = None
    self.parameters = None
    self.beginTime = None
    self.name = name
    self.remote = remote
    self.scenarioType = scenarioType
    self.dataDir = os.path.join(runPath, self.name)
    self.binary = os.path.join(DIR_PATH, SRC_DIR_PATH, BINARY_PATH.format(appName))
    try:
      self.expectedResult = ExpectedResultFiles(self.dataDir)
    except IOError:
      self.expectedResult = None # skip loading expected results when generating baseline files

  def __enter__(self):
    """
    Enter scenario object and create a temporary directory
    """
    self.beginTime = time.time()
    self._mkdtemp()
    self.parameters = ParameterFiles(self.dataDir, self.tempDir, self.remote)
    if self.expectedResult and self.benchmarkPaths:
      validateBenchmarks(self.baselineProfiles, len(self.benchmarkPaths))
    LOGGER.info('Running scenario %s | applog - %s\n', self.name, self.tempDir)
    return self

  def __exit__(self, excType, excVal, excTb):
    """
    Clean up temporary directories when exiting a scenario
    """
    elapsed = time.time() - self.beginTime
    LOGGER.completed('Running scenario %s completed in %0.2f sec.', self.name, elapsed)

  def makeTargetApp(self, context):
    """
    Deliver a target application to the remote host if running remotely
    """
    from xpedite.profiler.app       import TargetApp
    from xpedite.transport.remote   import deliver
    args = ([self.binary, '-c', '0', '-m', str(context.threadCount), '-t', str(context.txnCount)])
    targetApp = TargetApp(args)
    if self.remote:
      targetApp = deliver(self.remote.connection, targetApp)
    return targetApp

  def makeXpediteApp(self, workspace):
    """
    Create an Xpedite live profiling app
    """
    from xpedite.profiler.app import XpediteApp
    return XpediteApp(self.appName, self.appHost, self.appInfo, workspace=workspace)

  def makeXpediteDormantApp(self, runId, workspace, sampleFilePath=None):
    """
    Create an Xpedite dormant app (dry run without enabling probes)
    """
    from xpedite.profiler.app import XpediteDormantApp
    appInfo = os.path.join(
      self.dataDir, XPEDITE_APP_INFO_PARAMETER_PATH
    ) if sampleFilePath else os.path.join(self.tempDir, XPEDITE_APP_INFO_PATH)
    if sampleFilePath:
      xpediteApp = XpediteDormantApp(self.appName, LOCALHOST, appInfo, runId, workspace=workspace)
      xpediteApp.sampleFilePath = sampleFilePath
      return xpediteApp
    appHost = self.remote.host if self.remote else LOCALHOST
    return XpediteDormantApp(self.appName, appHost, appInfo, runId, workspace=workspace)

  def _mkdtemp(self):
    """
    Create a temporary directory for the scenario to use to store files
    """
    from test_xpedite import mkdtemp
    if self.remote:
      self.remote.connection.modules.sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
      remoteMkdtemp = rpyc.utils.classic.teleport_function(self.remote.connection, mkdtemp)
      self.tempDir = remoteMkdtemp()
      self.remote.connection.modules.os.chdir(self.tempDir)
    else:
      self.tempDir = mkdtemp()
      os.chdir(self.tempDir)

  def discoverRunId(self):
    """
    Collect data files from the test directory to determine the run ID
    """
    for fileName in os.listdir(os.path.join(self.dataDir, PARAMETERS_DATA_DIR)):
      if fileName.endswith(DATA_FILE_EXT):
        return fileName.split('-')[2]
    return None

  @property
  def appName(self):
    """Name of the application being profiled"""
    return self.parameters.profileInfo.appName

  @property
  def appHost(self):
    """Host the application is running on"""
    return self.parameters.profileInfo.appHost

  @property
  def appInfo(self):
    """specify application information"""
    return self.parameters.profileInfo.appInfo

  @property
  def profileInfo(self):
    """Profile information for the application being profiled"""
    return self.parameters.profileInfo

  @property
  def fullCpuInfo(self):
    """CPU information from an Xpedite app's environment"""
    return self.parameters.fullCpuInfo

  @property
  def probes(self):
    """Probes contained in the application being profiled"""
    return self.parameters.profileInfo.probes

  @property
  def benchmarkPaths(self):
    """Paths to benchmarks to compare current runs to when profiling"""
    return self.parameters.profileInfo.benchmarkPaths

  @property
  def baselineProfiles(self):
    """
    Profiles loaded from a baseline profile data file to compare
    against generated profile info
    """
    return self.expectedResult.baselineProfiles

  @property
  def cpuId(self):
    """The ID of the CPU used to generate expected results"""
    return self.expectedResult.baselineProfiles.cpuInfo.cpuId

  @property
  def baselineProbes(self):
    """Probe objects loaded from and baseline and unpickled"""
    return self.expectedResult.baselineProbes

  @property
  def baselineProfileInfo(self):
    """Profile information loaded from a baseline file"""
    return self.expectedResult.baselineProfileInfo

  @property
  def baselineProbeMap(self):
    """A map of probes used in the application"""
    return self.expectedResult.baselineProbeMap

class ScenarioLoader(object):
  """
  Scenario loader generates scenario objects
  1. Regular run
  2. Benchmark run
  for a given list of applications
  """

  def __init__(self):
    """
    Initialize scenario loader
    """
    from collections import OrderedDict
    self._scenarios = OrderedDict()
    self.remote = None

  def loadScenarios(self, runPath, apps, scenarioTypes=None, remote=None):
    """
    Load benchmark / regular scenarios for a list of applications
    """
    if not scenarioTypes:
      scenarioTypes = [ScenarioType.Regular, ScenarioType.Benchmark, ScenarioType.PMC]
    for app in apps:
      for scenarioType in scenarioTypes:
        scenario = Scenario(
          runPath, app, '{}{}'.format(app, scenarioType), scenarioType, remote
        )
        self._scenarios[scenario.name] = scenario
    return self

  def __getitem__(self, scenario):
    """Support indexing for scenario loader object"""
    return self._scenarios[scenario]

  def __iter__(self):
    """Support iterating over scenario loader objects"""
    for _, value in self._scenarios.items():
      yield value

  def keys(self):
    """Use scenario map keys as scenario loader keys"""
    return self._scenarios.keys()

  def items(self):
    """Use scenario map items as scenario loader items"""
    return self._scenarios.items()

  def values(self):
    """Use scenario map values as scenario loader values"""
    return self._scenarios.values()
