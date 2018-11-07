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
                                          loadProfileInfo
                                        )
from test_xpedite.test_profiler.profile import validateBenchmarks
from xpedite.jupyter                    import PROFILES_KEY

DIR_PATH = os.path.dirname(__file__)
SRC_DIR_PATH = os.path.join(DIR_PATH, '../../../..')
BINARY_PATH = 'install/test/{}'
PROFILER_PATH = 'scripts/bin/xpedite'

class ScenarioType(Enum):
  """Scenarios used in testing"""
  benchmark = 'benchmark'
  regular = 'regular'

class ParameterFiles(object):
  """
  Load and store parameters for a scenario
  """
  def __init__(self, dataDir, tempDir, remote=None):
    """
    Load input files for a scenario
    """
    baselineCpuInfoPath = os.path.join(dataDir, BASELINE_CPU_INFO_PATH)
    with open(baselineCpuInfoPath) as fileHandle:
      self.fullCpuInfo = json.load(fileHandle)
    appInfoPath = os.path.join(tempDir, XPEDITE_APP_INFO_PATH)
    self.profileInfo = loadProfileInfo(
      dataDir, PROFILE_INFO_PATH, appInfoPath=appInfoPath, remote=remote
    )

class ExpectedResultFiles(object):
  """
  Set expected results for a scenario
  """
  def __init__(self, dataDir, remote=None):
    """
    Load files with expected results for comparison
    """
    import cPickle as pickle
    from xpedite.jupyter.xpediteData    import XpediteDataReader
    baselineProbePath = os.path.join(dataDir, PROBE_CMD_BASELINE_PATH)
    with open(baselineProbePath) as probeFileHandle:
      self.baselineProbeMap = pickle.load(probeFileHandle)
    profileDataPath = os.path.join(dataDir, REPORT_CMD_BASELINE_PATH)
    with XpediteDataReader(profileDataPath) as xpediteDataReader:
      self.baselineProfiles = xpediteDataReader.getData(PROFILES_KEY)
    self.baselineProfileInfo = loadProfileInfo(dataDir, GENERATE_CMD_BASELINE_PATH)

class Scenario(object):
  """
  Load parameters and expected results information for a specific scenario of a test
  """
  def __init__(self, runPath, appName, name, remote=None):
    """
    Create a scenario and load parameters and expected results
    """
    from xpedite.jupyter.result     import Result
    from xpedite.transport.remote   import Remote
    from xpedite.util               import makeLogPath
    self.name = name
    self.dataDir = os.path.join(runPath, appName, self.name)
    self.remote = remote
    self.binary = os.path.join(DIR_PATH, SRC_DIR_PATH, BINARY_PATH.format(appName))
    self.result = Result()
    self.parameters = None
    self.expectedResult = None
    self.tempDir = None

  def __enter__(self):
    """
    Enter scenario object and create a temporary directory
    """
    self._mkdtemp()
    self.parameters = ParameterFiles(self.dataDir, self.tempDir, self.remote)
    self.expectedResult = ExpectedResultFiles(self.dataDir, self.remote)
    if self.benchmarkPaths:
      validateBenchmarks(self.baselineProfiles, len(self.benchmarkPaths))
    return self

  def __exit__(self, excType, excVal, excTb):
    """
    Clean up temporary directories when exiting a scenario
    """
    pass

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
    if sampleFilePath:
      appInfo = os.path.join(self.dataDir, XPEDITE_APP_INFO_PATH)
      appHost = 'localhost'
    else:
      appInfo = os.path.join(self.tempDir, XPEDITE_APP_INFO_PATH)
      appHost = self.remote.host if self.remote else 'localhost'
    xpediteApp = XpediteDormantApp(self.appName, appHost, appInfo, runId, workspace=workspace)
    if sampleFilePath:
      xpediteApp.sampleFilePath = sampleFilePath
    return xpediteApp

  def _mkdtemp(self):
    """
    Create a temporary directory for the scenario to use to store files
    """
    from test_xpedite import mkdtemp
    if self.remote:
      self.remote.connection.modules.sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
      rpyc.utils.classic.teleport_function(self.remote.connection, mkdtemp)
      self.tempDir = self.remote.connection.modules.os.getcwd()
    else:
      self.tempDir = mkdtemp()
      os.chdir(self.tempDir)

  def discoverRunId(self):
    """
    Collect data files from the test directory to determine the run ID
    """
    runId = None
    for fileName in os.listdir(self.dataDir):
      if fileName.endswith(DATA_FILE_EXT):
        words = fileName.split('-')
        runId = (words[2])
        return runId

  def generateProfileInfo(self, xpediteApp):
    """
    Generate profile information for a specific app to be compared to an expected result
    """
    from xpedite.profiler.profileInfoGenerator import ProfileInfoGenerator
    from xpedite.profiler.probeAdmin           import ProbeAdmin
    profiler = os.path.join(DIR_PATH, SRC_DIR_PATH, PROFILER_PATH)
    probes = ProbeAdmin.loadProbes(xpediteApp)
    generator = ProfileInfoGenerator(
      xpediteApp.executableName, xpediteApp.ip, xpediteApp.appInfoPath,
      probes, profiler
    )
    generator.generate()
    generatedProfileInfo = loadProfileInfo(self.dataDir, generator.filePath, self.remote)
    generatedProfileInfo.appInfo = XPEDITE_APP_INFO_PATH
    generatedProfileInfo.cpuSet = [0]
    generatedProfileInfo.benchmarkPaths = []
    generatedProfileInfo.appHost = 'localhost'
    return generatedProfileInfo

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

  def loadScenarios(self, runPath, apps, remote=None):
    """
    Load benchmark / regular scenarios for a list of applications
    """
    for app in apps:
      name = '{}_{}'.format(app, ScenarioType.regular.value)
      scenario = Scenario(runPath, app, name, remote)
      self._scenarios[scenario.name] = scenario
      name = '{}_{}'.format(app, ScenarioType.benchmark.value)
      scenario = Scenario(runPath, app, name, remote)
      self._scenarios[scenario.name] = scenario

  def __getitem__(self, scenario):
    """Support indexing for scenario loader object"""
    return self._scenarios[scenario]

  def __iter__(self):
    """Support iterating over scenario loader objects"""
    for _, value in self._scenarios.iteritems():
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
