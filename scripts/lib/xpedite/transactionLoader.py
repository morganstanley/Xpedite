"""
TransactionLoader

This module provides functionality to build transactions from a sequence of probes.
Two types of grouping are supported
  1. ChaoticTransactionLoader - builds transactions based on user supplied txnId
  2. BoundedTransactionLoader - builds transactions based on scopes and bounds

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.transaction     import Transaction, TransactionCollection
from xpedite.containers      import ProbeMap
from xpedite.txnFragment     import TxnFragments
from collections             import OrderedDict

class AbstractTransactionLoader(object):
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
    self.dataSources = []
    self.currentTransaction = None
    self.threadId = None
    self.tlsAddr = None

  def reset(self):
    """Resets the state of the loader"""
    self.processedCounterCount = 0
    self.transactions = OrderedDict()
    self.compromisedTransactions = []
    self.nonTransactionCounters = []
    self.ephemeralCounters = []
    self.currentTransaction = None

  def getTransactionCount(self):
    """Returns the number of transactions loaded"""
    return len(self.transactions)

  def isCompromised(self):
    """Returns True, if any of the loaded transactions were compromised or corrupted"""
    return len(self.compromisedTransactions) > 0

  def isNotAccounted(self):
    """Returns True, if any of the counters were skipped, due to data inconsistency"""
    return len(self.nonTransactionCounters) > 0

  def appendTransaction(self, txn):
    """
    Inserts or updates transaction to collection

    :param txn: Transaction to be appended

    """
    if txn.txnId in self.transactions:
      self.transactions[txn.txnId].join(txn)
    else:
      self.transactions.update({txn.txnId : txn})

  def report(self):
    """Returns loader statistics"""
    transactionCount = len(self.transactions) + len(self.compromisedTransactions)
    report = 'processed {:,} counters to build {:,} trasactions ({:,} intact / {:,} compromised)'.format(
      self.processedCounterCount, transactionCount, len(self.transactions), len(self.compromisedTransactions))
    if self.nonTransactionCounters:
      report += ' and {:,} were accounted extraneous'.format(len(self.nonTransactionCounters))
    else:
      report += '.'
    return report

  def beginCollection(self, dataSources):
    """
    Begins loading of samples from multiple threads of a target process

    Sets data sources for the current load session

    :param dataSource: Handle to source of loaded profile data

    """
    self.dataSources = dataSources

  def endCollection(self):
    """Ends loading of samples from multiple threads of a target process"""
    pass

  def beginLoad(self, threadId, tlsAddr):
    """Marks beginning of the current load session"""
    self.threadId = threadId
    self.tlsAddr = tlsAddr

  def endLoad(self):
    """Marks end of the current load session"""
    if self.currentTransaction:
      self.compromisedTransactions.append(self.currentTransaction)
      self.currentTransaction = None

  def getData(self):
    """Returns a collection of all the loaded transactions"""
    return TransactionCollection(
      self.name, self.cpuInfo, self.transactions, self.probes, self.topdownMetrics, self.events, self.dataSources
    )

  def getCount(self):
    """Returns the count of transactions loaded"""
    return len(self.transactions) + (1 if self.currentTransaction else 0)

class ChaoticTransactionLoader(AbstractTransactionLoader):
  """Loads transactions from counters with tolerance for compromised transactions"""

  def __init__(self, name, cpuInfo, probes, topdownMetrics, events):
    """
    Constructs a transaction loader resilient to data corruption

    :param name: Name of the profile being loaded
    :param probes: List of probes enabled for the profile session
    :param topdownMetrics: Top down metrics to be computed
    :param events: PMU events collected for the profile session
    """
    AbstractTransactionLoader.__init__(self, name, cpuInfo, probes, topdownMetrics, events)
    self.distortionCount = 0

  @staticmethod
  def markCounter(counter):
    """
    Extracts transaction id from the given counter

    :param counter: Counter with transaction id

    """
    if counter.data and len(counter.data) >= 8:
      idIndex = len(counter.data)-8
      counter.txnId = int(counter.data[idIndex:], 16)
      counter.data = counter.data[0:idIndex]
    return counter.txnId

  def loadCounter(self, counter):
    """
    Associates the given counter to a transaction

    :param counter: Counter with probe and sample data
    :type counter: xpedite.types.Counter

    """
    self.processedCounterCount += 1
    userProbe = self.probeMap.get(counter.probe, None)
    if self.currentTransaction:
      if userProbe:
        if not userProbe.isAnonymous:
          self.markCounter(counter)
        if counter.txnId or userProbe.isAnonymous:
          if userProbe.isAnonymous or counter.txnId == self.currentTransaction.txnId:
            self.currentTransaction.addCounter(counter, False)
          else:
            if self.distortionCount == 0:
              self.appendTransaction(self.currentTransaction)
              self.currentTransaction = Transaction(counter, counter.txnId)
            else:
              self.distortionCount -= 1
              self.compromisedTransactions.append(self.currentTransaction)
              self.currentTransaction = Transaction(counter, counter.txnId)
        else:
          # explicit probe missing id likely compromised - skip this and the next transaction
          self.currentTransaction.addCounter(counter, False)
          self.distortionCount = 2
      else:
        self.currentTransaction.addCounter(counter, False)
    elif userProbe and not userProbe.isAnonymous and self.markCounter(counter):
      self.currentTransaction = Transaction(counter, counter.txnId)
    else:
      self.nonTransactionCounters.append(counter)

  def endLoad(self):
    """Marks end of the current load session"""
    if self.currentTransaction:
      if self.distortionCount == 0:
        self.appendTransaction(self.currentTransaction)
      else:
        self.compromisedTransactions.append(self.currentTransaction)
    self.distortionCount = 0
    self.currentTransaction = None

class BoundedTransactionLoader(AbstractTransactionLoader):
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
    AbstractTransactionLoader.__init__(self, name, cpuInfo, probes, topdownMetrics, events)
    self.nextTxnId = 0
    self.ephemeralCounters = []
    self.nextFragmentId = 0
    self.fragments = TxnFragments()
    self.resumeFragment = None
    self.suspendingTxn = False

  def appendTransaction(self, txn):
    """
    Inserts or updates transaction to collection

    :param txn: Transaction to be appended

    """
    if not (self.resumeFragment or self.suspendingTxn):
      self.nextTxnId += 1
      txn.txnId = self.nextTxnId
      AbstractTransactionLoader.appendTransaction(self, txn)

  def loadCounter(self, counter):
    """
    Associates the given counter with a transaction

    :param counter: counter with probe and sample data
    :type counter: xpedite.types.Counter

    """
    self.processedCounterCount += 1
    userProbe = self.probeMap.get(counter.probe, counter.probe)
    if self.currentTransaction:
      if userProbe.canBeginTxn or userProbe.canResumeTxn:
        if self.currentTransaction.hasEndProbe or userProbe.canResumeTxn:
          if self.ephemeralCounters:
            self.nonTransactionCounters.extend(self.ephemeralCounters)
            self.ephemeralCounters = []
          self.appendTransaction(self.currentTransaction)
          self.currentTransaction = self.buildTransaction(counter, userProbe.canResumeTxn)
        else:
          self.currentTransaction.addCounter(counter, False)
      elif userProbe.canEndTxn or userProbe.canSuspendTxn:
        if self.ephemeralCounters:
          for eCounter in self.ephemeralCounters:
            self.currentTransaction.addCounter(eCounter, False)
          self.ephemeralCounters = []
        self.currentTransaction.addCounter(counter, True)
        if userProbe.canSuspendTxn:
          self.suspendingTxn |= userProbe.canSuspendTxn
          linkId = '{:x}{}'.format(counter.tsc, self.tlsAddr)
          self.fragments.addSuspendFragment(linkId, self.currentTransaction, self.resumeFragment)
      else:
        if self.currentTransaction.hasEndProbe:
          self.ephemeralCounters.append(counter)
        else:
          self.currentTransaction.addCounter(counter, False)
    else:
      if userProbe.canBeginTxn or userProbe.canResumeTxn:
        self.currentTransaction = self.buildTransaction(counter, userProbe.canResumeTxn)
        if self.ephemeralCounters:
          self.nonTransactionCounters.extend(self.ephemeralCounters)
          self.ephemeralCounters = []
      elif userProbe.canEndTxn or userProbe.canSuspendTxn:
        compromisedTransaction = self.buildTransaction(counter)
        for eCounter in self.ephemeralCounters:
          compromisedTransaction.addCounter(eCounter, False)
        self.compromisedTransactions.append(compromisedTransaction)
        self.ephemeralCounters = []
      else:
        self.ephemeralCounters.append(counter)

  def buildTransaction(self, counter, resumeTxn=False):
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
    if self.currentTransaction:
      if self.currentTransaction.hasEndProbe:
        self.appendTransaction(self.currentTransaction)
      else:
        self.compromisedTransactions.append(self.currentTransaction)
    self.currentTransaction = None

  def endCollection(self):
    """Ends loading of samples from multiple threads of a target process"""
    txns = self.fragments.join(self.nextTxnId)
    for txn in txns:
      AbstractTransactionLoader.appendTransaction(self, txn)
