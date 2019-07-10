"""
A Python module to store and load different testing scenarios for PMC related commands depending on CPU ID

Author: Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
from test_xpedite.test_pmu import (
                             EVENTS_DB_FILE_NAME, METRICS_FILE_NAME, TOPDOWN_FILE_NAME,
                             PMU_DATA, CPU_IDS
                           )

class PMUResults(object):
  """
  Set expected results for topdown, list, and metric commands for each CPU ID
  """
  def __init__(self, cpuId, rundir):
    """
    Load expected result files
    """
    self.cpuId = cpuId
    self.eventsDbBaseline = self.loadBaseline(EVENTS_DB_FILE_NAME, rundir)
    self.metricsBaseline = self.loadBaseline(METRICS_FILE_NAME, rundir)
    self.topdownBaseline = self.loadBaseline(TOPDOWN_FILE_NAME, rundir)

  def loadBaseline(self, fileName, rundir):
    """
    Load baseline files
    """
    filePath = os.path.join(rundir, PMU_DATA, self.cpuId, fileName)
    if not os.path.isfile(filePath):
      raise Exception('No {} baseline file exists for CPU ID {}'.format(fileName, self.cpuId))
    with open(filePath, 'r') as fileHandle:
      baseline = fileHandle.read()
    return baseline.split('\n')

class PMUResultsLoader(object):
  """
  Load PMU scenarios for all CPUs supported by xpedite
  """
  def __init__(self):
    """
    Create a dictionary of PMU events by CPU ID
    """
    from collections import OrderedDict
    self._pmuResults = OrderedDict()

  def loadPMUResults(self, rundir):
    """
    Load events for all CPUs
    """
    for cpuId in CPU_IDS:
      self._pmuResults[cpuId] = PMUResults(cpuId, rundir)

  def __getitem__(self, cpuId):
    """
    Support indexing
    """
    return self._pmuResults[cpuId]

  def __iter__(self):
    """
    Support iterating
    """
    for _, value in self._pmuResults.items():
      yield value
