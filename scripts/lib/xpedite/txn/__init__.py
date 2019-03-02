"""
Transaction and TransactionCollection

Transaction is any functionality in the program, that is a meaningful
target for profiling and optimisations.

A transaction stores data (timestamps and h/w counters) from a collection
of probes, that got hit, during program execution to achieve the functionality.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.util.probeFactory import ProbeIndexFactory

class Transaction(object):
  """
  Transaction is any functionality in the program, that is a meaningful target for profiling and optimisations.

  A transaction stores data (timestamps and h/w counters) from a collection of probes, that got hit, during program
  execution to achieve the functionality.
  """

  def __init__(self, counter, txnId):
    self.txnId = txnId
    self.counters = [counter]
    self.probeMap = None
    self.route = None
    self.begin = None
    self.end = None
    self.hasEndProbe = False

  def addCounter(self, counter, isEndProbe):
    """
    Adds the given counter to this transaction

    :param counter: Counter to be added
    :param isEndProbe: Flag to indicate, if the counter is sampled by an end probe

    """
    self.counters.append(counter)
    self.hasEndProbe = (self.hasEndProbe or isEndProbe)

  def join(self, other):
    """
    Adds counters from other transaction to self

    :param other: Transaction with counters to be added

    """
    self.counters = sorted(self.counters + other.counters, key=lambda counter: counter.tsc)

  def hasProbe(self, probe):
    """
    Checks if this transaction has the given probe

    :param probe: Probe to looup

    """
    return probe in self.probeMap

  def hasProbes(self, probes):
    """
    Checks if this transaction has the given list of probes

    :param probes: Collection of probes to lookup

    """
    for probe in probes:
      if probe not in self.probeMap:
        return False
    return True

  def hitCount(self, probe):
    """
    Returns the number of times a probe got hit in a txn

    :param probe: Probe to looup

    """
    return len(self.probeMap.get(probe, []))

  def getCounterForProbe(self, probe, index=0):
    """
    Returns the Nth counter for the given probe

    :param probe: Probe to lookup
    :param index: relative index for probes, that got hit many times (Default value = 0)

    """
    if probe in self.probeMap:
      return self.counters[self.probeMap[probe][index]]
    return None

  def getElapsedTsc(self):
    """Computes the elapsed time for this transaction"""
    return self.end.tsc - self.begin.tsc

  def finalize(self):
    """
    Marks the transaction as complete

    Populates the begin probe, end probe and route for this transaction
    """
    self.begin = min(self.counters, key=lambda c: c.tsc)
    self.end = max(self.counters, key=lambda c: c.tsc)
    index = ProbeIndexFactory.buildIndex(self.counters)
    self.route = index.route
    self.probeMap = index.probeMap

  def __getitem__(self, index):
    return self.counters[index]

  def __len__(self):
    """Returns the number of samples in a transaction"""
    return len(self.counters)

  def __iter__(self):
    return self.counters.__iter__()

  def __repr__(self):
    probeStr = ' -> '.join([counter.probe.getCanonicalName() for counter in self.counters])
    return 'Transaction: id {} | ({})'.format(self.txnId, probeStr)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__
