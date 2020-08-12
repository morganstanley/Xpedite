"""
Filters to exclude extraneous counters
This module provides implementation to exclude counters during transaction building.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

class TrivialCounterFilter(object):
  """Trivial nop filter"""

  @staticmethod
  def canLoad(counter):
    """
    Defaults to true for all non None counters

    :param counter: counter being loaded

    """
    return counter

  def reset(self):
    """Defaults to nop"""

  def report(self):
    """Defaults to nop"""

class AnonymousCounterFilter(object):
  """Filter to exclude counters missing txn id"""

  def __init__(self, probes, warmupThreshold=0):
    self.probeSet = set(probes)
    self.warmupThreshold = warmupThreshold
    self.reset()

  def isCounterInScope(self, counter):
    """
    Validates the following attributes for the given counter

    1. The couter carries data with valid txn id
    2. The counter originates from a known set of probes

    :param counter: counter being loaded

    """
    # ignore probes not in the probe list
    if counter.probe not in self.probeSet:
      self.extraneousCounters += 1
      return False

    if counter.txnId is None:
      self.nullIdCounters += 1
      return False
    if not counter.txnId:
      self.nullIdCounters += 1
      return False

    # ignore counter with transaction id's less thatn warmupThreshold for warmup
    txnId = counter.txnId
    if txnId <= self.warmupThreshold:
      self.warmupCounters += 1
      return False

    return True

  def canLoad(self, counter):
    """
    Checks for known counters with a valid txn id

    :param counter: counter being loaded

    """
    self.totalInspectedCounters += 1
    return self.isCounterInScope(counter)

  def reset(self):
    """Resets state of the filter"""
    self.extraneousCounters = 0
    self.nullIdCounters = 0
    self.totalInspectedCounters = 0
    self.warmupCounters = 0

  def report(self):
    """Returns statistics on the number of filtered counters"""
    totalFilteredCounters = self.extraneousCounters + self.nullIdCounters + self.warmupCounters
    if totalFilteredCounters - self.warmupCounters > 0:
      return (
        'Filtered {} out of total {} inspected counters are ignored for cause warmup({}) '
        '| NULL id ({}) | extraneous ({})'.format(
          totalFilteredCounters, self.totalInspectedCounters, self.warmupCounters,
          self.nullIdCounters, self.extraneousCounters
        )
      )
    return None

class BoundedTxnFilter(object):
  """Warmup filter for bounded transactions"""

  @staticmethod
  def filter(transactions, beginProbe, endProbe, warmupThreshold=0):
    """
    Filters the first N transactions to ignore side effects of application warmup

    :param transactions: List of loaded transactions
    :param beginProbe: Begin probe for the transactions
    :param endProbe: End probe for the transactions
    :param warmupThreshold: Number of warmup transactions to ignore (Default value = 0)

    """
    probeSet = set(beginProbe, endProbe)
    return [txn for txn in transactions[warmupThreshold:] if txn.has(probeSet)]
