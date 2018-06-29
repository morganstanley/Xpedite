"""
Transaction and TransactionCollection

Transaction is any functionality in the program, that is a meaningful
target for profiling and optimisations.

A transaction stores data (timestamps and h/w counters) from a collection
of probes, that got hit, during program execution to achieve the functionality.

TransactionCollection is a collection of all transaction in a profiling session.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from collections  import OrderedDict
from xpedite.probeFactory import ProbeIndexFactory

class TransactionSubCollection(object):
  """A subset of transactions in a transaction collection"""

  def __init__(self, name, cpuInfo, transactions, probes, topdownMetrics, events):
    self.name = name
    self.cpuInfo = cpuInfo
    self.transactions = transactions
    self.probes = probes
    self.topdownMetrics = topdownMetrics
    self.events = events

  def __getitem__(self, index):
    return self.transactions[index]

  def __len__(self):
    return len(self.transactions)

  def append(self, transaction):
    """
    Appends transaction to this sub collection

    :param transaction: Transaction to be added

    """
    return self.transactions.append(transaction)

  def cloneMetaData(self):
    """Creates a empty sub collection object with cloned meta data"""
    return TransactionSubCollection(self.name, self.cpuInfo, [], self.probes, self.topdownMetrics, self.events)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class TransactionCollection(object):
  """A collection of transactions sharing a common route"""

  def __init__(self, name, cpuInfo, transactionsMap, probes, topdownMetrics, events, dataSources):
    self.name = name
    self.cpuInfo = cpuInfo
    for txn in transactionsMap.values():
      txn.finalize()
    self.transactionsMap = OrderedDict(sorted(transactionsMap.iteritems(), key=lambda pair: pair[1].begin.tsc))
    if len(self.transactionsMap) != len(transactionsMap):
      raise Exception('failed to reorder transaction for {}'.format(name))
    self.probes = probes
    self.topdownMetrics = topdownMetrics
    self.events = events
    self.dataSources = dataSources
    self.repo = None

  def getSubCollection(self):
    """Builds an instance of transaction sub collection"""
    return TransactionSubCollection(
      self.name, self.cpuInfo, self.transactionsMap.values(), self.probes, self.topdownMetrics, self.events
    )

  def isCurrent(self):
    """Returns Ture, if this collection has transactions for the current profiling session"""
    return self.repo.getCurrent() is self if self.repo else False

  def __iter__(self):
    for txn in self.transactionsMap.values():
      yield txn

  def __repr__(self):
    rep = ''
    for txn in self:
      rep += '{}\n'.format(txn)
    return rep

  def __eq__(self, other):
    selfDict = dict((k, val) for k, val in self.__dict__.iteritems() if k != 'repo' and k != 'dataSources')
    otherDict = dict((k, val) for k, val in other.__dict__.iteritems() if k != 'repo' and k != 'dataSources')
    return selfDict == otherDict

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

  def getCounterForProbe(self, probe, index=0):
    """
    Returns the Nth counter for the given probe

    :param probe: Probe to lookup
    :param index: relative index for probes, that got hit many times (Default value = 0)

    """
    if probe in self.probeMap:
      return self.counters[self.probeMap[probe][index]]

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
