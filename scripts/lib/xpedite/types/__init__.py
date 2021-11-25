"""
Class definitions used in transaction building

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
from xpedite.dependencies import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Enum, Package.Six)
from enum import Enum # pylint: disable=wrong-import-position

class Counter(object):

  """
  Stores the profile data (timing/pmc) collected by probes

  A counter can store cpu time stamp counter (tsc) and a collection
  of pmc values collected by any of the core and offcore pmu units
  """

  def __init__(self, threadId, probe, data, tsc):
    self.threadId = threadId
    self.probe = probe
    data = data.strip()
    self.txnId = None
    self.data = data
    self.tsc = tsc
    self.pmcs = []

  def addPmc(self, pmc):
    """
    Adds the given pmc value to the counter

    :param pmc: value of the pmc event

    """
    self.pmcs.append(pmc)

  @property
  def name(self):
    """Returns the sysName of this coutner's probe"""
    return self.probe.sysName

  def markAnonymous(self):
    """Clears the txnId of this counter"""
    self.txnId = None

  def __repr__(self):
    rep = 'Counter: {}'.format(str(self.probe))
    if self.txnId:
      rep += ' | id {}'.format(self.txnId)
    if self.pmcs:
      rep += ' | {} pmc counters'.format(len(self.pmcs))
    return rep

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class ResultOrder(Enum):
  """Sort order of transactions in latency constituent reports"""

  Chronological = 1
  WorstToBest = 2
  BestToWorst = 3
  TransactionId = 4

  def __eq__(self, other):
    if other:
      return self.__dict__ == other.__dict__
    return None

class RouteConflation(Enum):
  """Sort order of transactions in latency constituent reports"""

  On = 1
  Off = 2

  def __eq__(self, other):
    if other:
      return self.__dict__ == other.__dict__
    return None

class CpuInfo(object):
  """Info about cpu model and configuration"""

  def __init__(self, cpuId, frequency):
    self.cpuId = cpuId
    self.frequency = frequency
    self.frequencyKhz = frequency / 1000
    self.cyclesPerUsec = float(self.frequencyKhz / 1000)

  def convertCyclesToTime(self, cycles):
    """
    Converts cpu time stamp counter cycles to wall time

    :param cycles: cpu time stamp counter (tsc)

    """
    return cycles / self.cyclesPerUsec

  def __repr__(self):
    return 'id - {} | freq {} | {} Khz | {} Usec'.format(
      self.cpuId, self.frequency, self.frequencyKhz, self.cyclesPerUsec
    )

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class InvariantViloation(Exception):
  """Exception for fatal invariant violations"""

  def __init__(self, violation):
    Exception.__init__(self, violation)
