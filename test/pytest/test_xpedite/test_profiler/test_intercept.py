"""
A pytest module to test intercept functionality
"""

import pytest
from test_xpedite.test_profiler.profile       import runXpediteRecord
from test_xpedite.test_profiler.context       import Context
from test_xpedite.test_profiler.scenario      import ScenarioLoader, ScenarioType

CONTEXT = None
SCENARIO_LOADER = ScenarioLoader()
ALLOCATOR_APP = ['allocatorApp']

@pytest.fixture(autouse=True)
def setTestParameters(transactions, multithreaded, workspace, rundir):
  """
  A method run at the beginning of tests to set test context variables
  """
  global CONTEXT # pylint: disable=global-statement
  CONTEXT = Context(transactions, multithreaded, workspace)
  scenarioTypes = [ScenarioType.Regular, ScenarioType.Benchmark]
  SCENARIO_LOADER.loadScenarios(rundir, ALLOCATOR_APP, scenarioTypes)

def test_intercept(capsys):
  """
  Run and test intercept functionality with a target doing memory allocations

  @param capsys: A pytest fixture allowing disabling of I/O capturing
  """
  for scenarios in SCENARIO_LOADER:
    with scenarios as scenarios:
      with capsys.disabled():
        report, _, _ = runXpediteRecord(CONTEXT, scenarios)
        profiles = report.profiles
        assert len(profiles) == 1
        timelines = profiles[0].current.timelineCollection
        assert len(timelines) == int(CONTEXT.txnCount)
        for tl in timelines:
          assert tl.txn is not None
          for probe in scenarios.probes:
            assert tl.txn.hasProbe(probe)
