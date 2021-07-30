"""
Transaction Loader

This module provides functionality to build transactions from a sequence of probes.
The BoundedTxnLoader builds transactions based on scopes and bounds.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.txn                 import Transaction
from xpedite.txn.collection      import TxnCollection
from xpedite.types.containers    import ProbeMap
from xpedite.txn.fragments       import TxnFragments
from collections                 import OrderedDict

class AbstractTxnLoader(object):
  """Base class for building transactions from counters"""

  def __init__(self, name, cpuInfo, probes, topdownMetrics, events):
    """
    Constructs a abstract transaction loader

    :param name: Name of the profile being loaded
    :param probes: List of probes enabled for the profile session
    :param topdownMetrics: Top down metrics to be computed
    :param events: PMU events collected for the profile session
    """
    self.name = name
    self.cpuInfo = cpuInfo
    self.probes = probes
    self.topdownMetrics = topdownMetrics
    self.events = events
    self.probeMap = ProbeMap(probes)
    self.reset()
    self.dataSource = None
    self.currentTxn = None
    self.threadId = None
    self.tlsAddr = None

  def reset(self):
    """Resets the state of the loader"""
    self.processedCounterCount = 0
    self.txns = OrderedDict()
    self.compromisedTxns = []
    self.nonTxnCounters = []
    self.ephemeralCounters = []
    self.currentTxn = None

  def getTxnCount(self):
    """Returns the number of transactions loaded"""
    return len(self.txns)

  def isCompromised(self):
    """Returns True, if any of the loaded transactions were compromised or corrupted"""
    return len(self.compromisedTxns) > 0

  def isNotAccounted(self):
    """Returns True, if any of the counters were skipped, due to data inconsistency"""
    return len(self.nonTxnCounters) > 0

  def appendTxn(self, txn):
    """
    Inserts or updates transaction to collection

    :param txn: Transaction to be appended

    """
    if txn.txnId in self.txns:
      self.txns[txn.txnId].join(txn)
    else:
      self.txns.update({txn.txnId : txn})

  def report(self):
    """Returns loader statistics"""
    txnCount = len(self.txns) + len(self.compromisedTxns)
    report = 'processed {:,} counters to build {:,} trasactions ({:,} intact / {:,} compromised)'.format(
      self.processedCounterCount, txnCount, len(self.txns), len(self.compromisedTxns))
    if self.nonTxnCounters:
      report += ' and {:,} were accounted extraneous'.format(len(self.nonTxnCounters))
    else:
      report += '.'
    return report

  def beginCollection(self, dataSource):
    """
    Begins loading of samples from multiple threads of a target process

    Sets data sources for the current load session

    :param dataSource: Handle to source of loaded profile data

    """
    self.dataSource = dataSource

  def endCollection(self):
    """Ends loading of samples from multiple threads of a target process"""

  def beginLoad(self, threadId, tlsAddr):
    """Marks beginning of the current load session"""
    self.threadId = threadId
    self.tlsAddr = tlsAddr

  def endLoad(self):
    """Marks end of the current load session"""
    if self.currentTxn:
      self.compromisedTxns.append(self.currentTxn)
      self.currentTxn = None

  def getData(self):
    """Returns a collection of all the loaded transactions"""
    return TxnCollection(
      self.name, self.cpuInfo, self.txns, self.probes, self.topdownMetrics, self.events, self.dataSource
    )

  def getCount(self):
    """Returns the count of transactions loaded"""
    return len(self.txns) + (1 if self.currentTxn else 0)

class BoundedTxnLoader(AbstractTxnLoader):
  """Loads transactions bounded by well defined begin/end probes"""

  def __init__(self, name, cpuInfo, probes, topdownMetrics, events):
    """
    Constructs a loader, that builds transactions based on probe bounds (begin/end probes)

    :param name: Name of the profiling session
    :param cpuInfo: Cpu info of the host running target app
    :param probes: List of probes enabled for the profiling session
    :param topdownMetrics: Top down metrics to be computed
    :param events: PMU events collected for the profile session

    """
    AbstractTxnLoader.__init__(self, name, cpuInfo, probes, topdownMetrics, events)
    self.nextTxnId = 0
    self.ephemeralCounters = []
    self.nextFragmentId = 0
    self.fragments = TxnFragments()
    self.resumeFragment = None
    self.suspendingTxn = False

  def appendTxn(self, txn):
    """
    Inserts or updates transaction to collection

    :param txn: Transaction to be appended

    """
    if not (self.resumeFragment or self.suspendingTxn):
      self.nextTxnId += 1
      txn.txnId = self.nextTxnId
      AbstractTxnLoader.appendTxn(self, txn)

  def loadCounter(self, counter):
    """
    Associates the given counter with a transaction

    :param counter: counter with probe and sample data
    :type counter: xpedite.types.Counter

    """
    self.processedCounterCount += 1
    userProbe = self.probeMap.get(counter.probe, counter.probe)
    if self.currentTxn:
      if userProbe.canBeginTxn or userProbe.canResumeTxn:
        if self.currentTxn.hasEndProbe or userProbe.canResumeTxn:
          if self.ephemeralCounters:
            self.nonTxnCounters.extend(self.ephemeralCounters)
            self.ephemeralCounters = []
          self.appendTxn(self.currentTxn)
          self.currentTxn = self.buildTxn(counter, userProbe.canResumeTxn)
        else:
          self.currentTxn.addCounter(counter, False)
      elif userProbe.canEndTxn or userProbe.canSuspendTxn:
        if self.ephemeralCounters:
          for eCounter in self.ephemeralCounters:
            self.currentTxn.addCounter(eCounter, False)
          self.ephemeralCounters = []
        self.currentTxn.addCounter(counter, True)
        if userProbe.canSuspendTxn:
          self.suspendingTxn |= userProbe.canSuspendTxn
          linkId = '{:x}{}'.format(counter.tsc, self.tlsAddr)
          self.fragments.addSuspendFragment(linkId, self.currentTxn, self.resumeFragment)
      else:
        if self.currentTxn.hasEndProbe:
          self.ephemeralCounters.append(counter)
        else:
          self.currentTxn.addCounter(counter, False)
    else:
      if userProbe.canBeginTxn or userProbe.canResumeTxn:
        self.currentTxn = self.buildTxn(counter, userProbe.canResumeTxn)
        if self.ephemeralCounters:
          self.nonTxnCounters.extend(self.ephemeralCounters)
          self.ephemeralCounters = []
      elif userProbe.canEndTxn or userProbe.canSuspendTxn:
        compromisedTxn = self.buildTxn(counter)
        for eCounter in self.ephemeralCounters:
          compromisedTxn.addCounter(eCounter, False)
        self.compromisedTxns.append(compromisedTxn)
        self.ephemeralCounters = []
      else:
        self.ephemeralCounters.append(counter)

  def buildTxn(self, counter, resumeTxn=False):
    """
    Constructs a new transaction instance

    :param counter: counter with probe and sample data

    """
    txnId = None
    self.suspendingTxn = False
    if resumeTxn:
      txnId = counter.data
      txn = Transaction(counter, txnId)
      self.resumeFragment = self.fragments.addResumeFragment(txnId, txn)
      return txn
    self.resumeFragment = None
    self.nextFragmentId += 1
    return Transaction(counter, self.nextFragmentId)

  def endLoad(self):
    """Marks end of the current load session"""
    if self.currentTxn:
      if self.currentTxn.hasEndProbe:
        self.appendTxn(self.currentTxn)
      else:
        self.compromisedTxns.append(self.currentTxn)
    self.currentTxn = None

  def endCollection(self):
    """Ends loading of samples from multiple threads of a target process"""
    txns = self.fragments.join(self.nextTxnId)
    for txn in txns:
      AbstractTxnLoader.appendTxn(self, txn)
