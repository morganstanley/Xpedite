"""
Pytest module to test xpedite features report and record with:
Application running in a REMOTE box, benchmarks and performance counters
This module also provides test for generating profile information and
a Jupyter notebook

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
import pytest
import logging
import logging.config
from logger                                import LOG_CONFIG_PATH
logging.config.fileConfig(LOG_CONFIG_PATH)
from test_xpedite.test_profiler.profile    import (
                                             loadProfileInfo, runXpediteReport, runXpediteRecord,
                                             generateProfileInfo, loadProbes, buildNotebook,
                                             compareAgainstBaseline
                                           )
from test_xpedite.test_profiler.comparator import findDiff

TEST_DIR = os.path.abspath(os.path.dirname(__file__))
DATA_DIR = os.path.join(TEST_DIR, '..', 'data')
REMOTE = None
TXN_COUNT = None
THREAD_COUNT = None
WORKSPACE = ''

FIX_DECODER_BINARY = os.path.join(os.path.dirname(__file__), '../../../..', 'install/test/slowFixDecoder')
ALLOCATOR_BINARY = os.path.join(os.path.dirname(__file__), '../../../..', 'install/test/allocatorApp')

LOGGER = logging.getLogger('xpedite')

@pytest.fixture(autouse=True)
def setTestParameters(hostname, transactions, multithreaded, workspace):
  """
  A method run at the beginning of tests to create and enter a REMOTE environment
  and at the end of tests to exit the REMOTE environment

 @param hostname: An option added to the pytest parser to accept a REMOTE host
                   Defaults to 127.0.0.1
  @type hostname: C{str}
  """
  from xpedite.util              import makeLogPath
  from xpedite.transport.net     import isIpLocal
  from xpedite.transport.remote  import Remote
  global REMOTE, TXN_COUNT, THREAD_COUNT, WORKSPACE # pylint: disable=global-statement
  if not isIpLocal(hostname):
    REMOTE = Remote(hostname, makeLogPath('remote'))
    REMOTE.__enter__()
  TXN_COUNT = transactions
  THREAD_COUNT = multithreaded
  WORKSPACE = workspace
  yield
  if REMOTE:
    REMOTE.__exit__(None, None, None)

def test_report_against_baseline():
  """
  Run xpedite report on a data file in the test directory, return profiles and compare
  the previously generated profiles from the same xpedite run
  """
  profileInfoPath = os.path.join(DATA_DIR, 'profileInfo.py')
  baselinePath = os.path.join(DATA_DIR, 'reportCmdData/reportCmdBaseline.xpd')
  compareAgainstBaseline(profileInfoPath, baselinePath, workspace=WORKSPACE)

def test_benchmark_report_against_baseline():
  """
  Run xpedite report on a data file in the test directory, return profiles and compare
  the previously generated profiles from the same xpedite run
  """
  profileInfoPath = os.path.join(DATA_DIR, 'profileInfoWithBenchmark.py')
  baselinePath = os.path.join(DATA_DIR, 'reportCmdData/reportCmdBaselineWithBenchmark.xpd')
  compareAgainstBaseline(profileInfoPath, baselinePath, workspace=WORKSPACE)

def test_record_against_report(capsys, profileInfoPath=None):
  """
  Run xpedite record and xpedite report to compare profiles

  @param capsys: A pytest fixture allowing disabling of I/O capturing
  @param profileInfoPath: Override default profileInfo.py file from the test data directory
  """
  profileInfoPath = profileInfoPath if profileInfoPath else 'profileInfo.py'
  profileInfo = loadProfileInfo(profileInfoPath, REMOTE)
  with capsys.disabled():
    app, recordProfiles, _ = runXpediteRecord(FIX_DECODER_BINARY, profileInfo, TXN_COUNT, THREAD_COUNT, REMOTE, WORKSPACE)
  profileInfo = loadProfileInfo(profileInfoPath, REMOTE)
  profileInfo.appInfo = os.path.join(app.tempDir, 'xpedite-appinfo.txt')
  reportProfiles, _ = runXpediteReport(app.xpediteApp.runId, profileInfo, workspace=WORKSPACE)
  findDiff(reportProfiles.__dict__, recordProfiles.__dict__)
  assert reportProfiles == recordProfiles

@pytest.mark.pmc
def test_pmc_record_against_report(capsys):
  """
  Run xpedite record and xpedite report with performance counters
  """
  test_record_against_report(capsys, profileInfoPath='pmcProfileInfo.py')

def test_intercept(capsys):
  """
  Run and test intercept functionality with a target doing memory allocations

  @param capsys: A pytest fixture allowing disabling of I/O capturing
  """
  from xpedite import Probe
  profileInfo = loadProfileInfo('allocatorProfileInfo.py', REMOTE)
  with capsys.disabled():
    _, profiles, _ = runXpediteRecord(ALLOCATOR_BINARY, profileInfo, TXN_COUNT, THREAD_COUNT, REMOTE)
  assert len(profiles) == 1
  timelines = profiles[0].current.timelineCollection
  assert len(timelines) == int(TXN_COUNT)
  for tl in timelines:
    assert tl.txn is not None
    for probe in profileInfo.probes:
      assert tl.txn.hasProbe(probe)

def test_generate_against_baseline():
  """
  Test xpedite generate by generating a new profileInfo.py file and comparing to baseline
  profileInfo.py in the test data directory
  """
  import tempfile
  profileInfoBaseline = os.path.join(DATA_DIR, 'generateCmdBaseline.py')
  baselineProfileInfo = loadProfileInfo(profileInfoBaseline, REMOTE)
  tempDir = tempfile.mkdtemp()
  os.chdir(tempDir)
  profileInfo = loadProfileInfo('profileInfo.py', REMOTE)
  tempProfilePath = generateProfileInfo(FIX_DECODER_BINARY, profileInfo, TXN_COUNT, THREAD_COUNT, REMOTE, WORKSPACE)
  tempProfileInfo = loadProfileInfo(tempProfilePath, REMOTE)
  tempProfileInfo.appInfo = os.path.basename(tempProfileInfo.appInfo)
  (tempProfileInfo.appHost, baselineProfileInfo.appHost) = ('localhost', 'localhost')
  findDiff(tempProfileInfo.__dict__, baselineProfileInfo.__dict__)
  assert tempProfileInfo == baselineProfileInfo

def test_probe_states(capsys):
  """
  Test xpedite probes and probes state for an application's against baseline probe states
  for the application
  """
  import cPickle as pickle
  from xpedite.types.probe import compareProbes
  profileInfo = loadProfileInfo(os.path.join(DATA_DIR, 'profileInfo.py'), REMOTE)
  probes = []
  probeMap = {}
  baselineProbeMap = {}
  with capsys.disabled():
    probes = loadProbes(FIX_DECODER_BINARY, profileInfo, TXN_COUNT, THREAD_COUNT, REMOTE, WORKSPACE)
    for probe in probes:
      probeMap[probe.sysName] = probe

  with open(os.path.join(DATA_DIR, 'probeCmdBaseline.pkl'), 'r') as probeFileHandle:
    baselineProbes = pickle.load(probeFileHandle)
    for probe in baselineProbes:
      baselineProbeMap[probe.sysName] = probe

  assert len(probes) == len(baselineProbes)
  for probe in probeMap.keys():
    findDiff(probeMap, baselineProbeMap)
    assert compareProbes(probeMap[probe], baselineProbeMap[probe])

def test_notebook_build(capsys):
  """
  Test to confirm a Jupyter notebook can be creating from profile information and results
  generated by xpedite record
  """
  with capsys.disabled():
    notebook, _, _, result, _ = buildNotebook(FIX_DECODER_BINARY, TXN_COUNT, THREAD_COUNT, REMOTE)
    assert len(result.reportCells) > 0
    assert notebook
