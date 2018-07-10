"""
Pytest module to test xpedite features report and record with:
Application running in a remote box, benchmarks and performance counters
This module also provides test for generating profile information and
a Jupyter notebook

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
import pytest
import logging
import logging.config
from logger import LOG_CONFIG_PATH
logging.config.fileConfig(LOG_CONFIG_PATH)
from test_xpedite.test_profiler.profile  import (
                                           loadProfileInfo, runXpediteReport, runXpediteRecord,
                                           loadBaseline, collectDataFiles, generateProfileInfo,
                                           loadProbes, buildNotebook
                                         )

testDir = os.path.abspath(os.path.dirname(__file__))
dataDir = os.path.join(testDir, '..', 'data')
remote = None

DEMO_BINARY = os.path.join(os.path.dirname(__file__), '../../../..', 'install/bin/xpediteDemo')
ALLOCATOR_BINARY = os.path.join(os.path.dirname(__file__), '../../../..', 'install/test/allocatorApp')

LOGGER = logging.getLogger()

@pytest.fixture(autouse=True)
def set_hostname(hostname):
  """
  A method run at the beginning of tests to create and enter a remote environment
  and at the end of tests to exit the remote environment

  @param hostname: An option added to the pytest parser to accept a remote host
                   Defaults to 127.0.0.1
  @type hostname: C{str}
  """
  from xpedite.util              import makeLogPath
  from xpedite.transport.net     import isIpLocal
  from xpedite.transport.remote  import Remote
  global remote
  if not isIpLocal(hostname):
    remote = Remote(hostname, makeLogPath('remote'))
    remote.__enter__()
  yield
  if remote:
    remote.__exit__(None, None, None)

def test_report_against_baseline():
  """
  Run xpedite report on a data file in the test directory, return profiles and compare
  the previously generated profiles from the same xpedite run
  """
  from test_xpedite.test_profiler.comparator import findDiff
  profileInfo = loadProfileInfo('profileInfo.py')
  for runId, dataFilePath in collectDataFiles().iteritems():
    reportProfiles, _ = runXpediteReport(runId, profileInfo, dataFilePath)
    reportProfiles.transactionRepo = None # transactionRepo not stored in .xpd files
    baselineProfiles = loadBaseline()
    findDiff(reportProfiles.__dict__, baselineProfiles.__dict__)
    assert reportProfiles == baselineProfiles

def test_record_against_report(capsys, profileInfoPath=None):
  """
  Run xpedite record and xpedite report to compare profiles

  @param capsys: A pytest fixture allowing disabling of I/O capturing
  @param profileInfoPath: Override default profileInfo.py file from the test data directory
  """
  from test_xpedite.test_profiler.comparator import findDiff
  profileInfoPath = profileInfoPath if profileInfoPath else 'profileInfo.py'
  profileInfo = loadProfileInfo(profileInfoPath, remote)
  with capsys.disabled():
    app, recordProfiles, _ = runXpediteRecord(DEMO_BINARY, profileInfo, remote)
  profileInfo = loadProfileInfo(profileInfoPath, remote)
  profileInfo.appInfo = os.path.join(app.tempDir, 'xpedite-appinfo.txt')
  reportProfiles, _ = runXpediteReport(app.xpediteApp.runId, profileInfo)
  findDiff(reportProfiles.__dict__, recordProfiles.__dict__)
  assert recordProfiles == reportProfiles

def test_benchmarks(capsys):
  """
  Run xpedite record and xpedite report with benchmark profile information
  """
  test_record_against_report(capsys, profileInfoPath='benchmarkProfileInfo.py')

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
  profileInfo = loadProfileInfo('allocatorProfileInfo.py', remote)
  with capsys.disabled():
    app, profiles, _ = runXpediteRecord(ALLOCATOR_BINARY, profileInfo, remote)
  assert len(profiles) == 1
  timelines = profiles[0].current.timelineCollection
  assert len(timelines) == 100
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
  profileInfoBaseline = os.path.join(dataDir, 'baselineProfileInfo.py')
  baselineProfileInfo = loadProfileInfo(profileInfoBaseline, remote)
  tempDir = tempfile.mkdtemp()
  os.chdir(tempDir)
  profileInfo = loadProfileInfo('profileInfo.py', remote)
  tempProfilePath = generateProfileInfo(DEMO_BINARY, profileInfo, remote)
  tempProfileInfo = loadProfileInfo(tempProfilePath, remote)
  tempProfileInfo.appHost = 'localhost'
  baselineProfileInfo.appHost = 'localhost'
  tempProfileInfo.cpuSet = [0]
  assert tempProfileInfo == baselineProfileInfo

def test_probe_states(capsys):
  """
  Test xpedite probes and probes state for an application's against baseline probe states
  for the application
  """
  import cPickle as pickle
  from xpedite.types.probe import compareProbes
  profileInfo = loadProfileInfo(os.path.join(dataDir, 'profileInfo.py'), remote)
  probes = []
  with capsys.disabled():
    probes = loadProbes(DEMO_BINARY, profileInfo, remote)

  with open(os.path.join(dataDir, 'probeBaseline.pkl'), 'r') as probeFileHandle:
    baselineProbes = pickle.load(probeFileHandle)

  assert len(probes) == len(baselineProbes)
  for i in range(len(probes)):
    assert compareProbes(probes[i], baselineProbes[i]);

def test_notebook_build(capsys):
  """
  Test to confirm a Jupyter notebook can be creating from profile information and results
  generated by xpedite record
  """
  with capsys.disabled():
    notebook, _, _, result = buildNotebook(DEMO_BINARY, remote)
    assert len(result.reportCells) > 0
    assert notebook
