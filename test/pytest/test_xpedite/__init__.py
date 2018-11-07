"""
This package contains pytests for Xpedite profiling including:

1. test_report_vs_baseline
   Test Xpedite's report command against a previously generated Xpedite data file
2. test_record_vs_report
   Compare results from Xpedite's record and report commands and ensure they match
3. test_generate_cmd_vs_baseline
   Test Xpedite's generate command (to automatically generate profile information)
   by comparing new profile information to previously generated profile information
4. test_probe_states
   Test an application's probe states against probe states loaded from a previously
   generated file with pickled probe objects
5. test_notebook_build
   Test building a Jupyter notebook by checking that a Jupyter notebook has been
   generated and it has at least 1 report cell

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

REPORT_CMD_BASELINE_PATH = 'reportCmdBaseline.xpd'
PROFILE_INFO_PATH = 'profileInfo.py'
XPEDITE_APP_INFO_PATH = 'xpedite-appinfo.txt'
DATA_FILE_EXT = '.data'
BASELINE_CPU_INFO_PATH = 'baselineCpuInfo.json'
PROBE_CMD_BASELINE_PATH = 'probeCmdBaseline.pkl'
GENERATE_CMD_BASELINE_PATH = 'generateCmdBaseline.py'
TXN_COUNT = 1024
THREAD_COUNT = 1

def loadProfileInfo(dataDir, profileInfoPath, appInfoPath=None, remote=None):
  """
  Load profile information from a profileInfo.py file, set a default application information
  file, and set the profile information's host if the application is running remotely

  @param remote: Remote environment information if a remote host is passed to the pytest parser
  @type remote: C{xpedite.transport.remote.Remote}
  """
  import os
  import xpedite.profiler.profileInfo
  profileInfo = xpedite.profiler.profileInfo.loadProfileInfo(os.path.join(dataDir, profileInfoPath))
  if appInfoPath:
    profileInfo.appInfo = appInfoPath
  if remote:
    profileInfo.appHost = remote.host
  if profileInfo.benchmarkPaths:
    profileInfo.benchmarkPaths = [os.path.dirname(dataDir)]
  return profileInfo

def mkdtemp():
  """
  Create and clean a temporary directory
  """
  import os
  import sys
  import tempfile
  import test_xpedite.test_profiler
  tempDir = tempfile.mkdtemp()
  for dataFile in os.listdir(tempDir):
    os.remove(dataFile)
  return tempDir
