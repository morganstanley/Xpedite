"""

Tests for PMC related Xpedite commands
This module parametrizes tests by CPU ID

Author: Brooke Elizabeth Cantwell, Morgan Stanley

"""

import pytest
from xpedite.pmu.topdown              import Topdown
from xpedite.pmu.eventsDb             import loadEventsDb
from test_xpedite.test_pmu.pmuResults import PMUResultsLoader

PMU_RESULTS_LOADER = None

@pytest.fixture(scope='module', autouse=True)
def setTestParameters(rundir):
  """
  Set variables needed for testing
  """
  global PMU_RESULTS_LOADER # pylint: disable=global-statement
  PMU_RESULTS_LOADER = PMUResultsLoader()
  PMU_RESULTS_LOADER.loadPMUResults(rundir)

def test_list_command(cpuId):
  """
  Test listing contents of eventsDb
  """
  pmuResults = PMU_RESULTS_LOADER[cpuId]
  eventsDb = loadEventsDb(cpuId)
  events = '\n'.join([str(event) for event in eventsDb.eventsMap.values()])
  assert events.split('\n') == pmuResults.eventsDbBaseline[:-1]

def test_metrics_command(cpuId):
  """
  Test generating topdown metrics display
  """
  pmuResults = PMU_RESULTS_LOADER[cpuId]
  topdown = Topdown(loadEventsDb(cpuId))
  metrics = '\n'.join([topdown.metricsToString(name) for name in topdown.metrics()])
  assert metrics.split('\n') == pmuResults.metricsBaseline[:-1]

def test_topdown_command(cpuId):
  """
  Test display of topdown hierarchy
  """
  pmuResults = PMU_RESULTS_LOADER[cpuId]
  topdown = Topdown(loadEventsDb(cpuId))
  assert str(topdown.hierarchy).split('\n') == pmuResults.topdownBaseline
