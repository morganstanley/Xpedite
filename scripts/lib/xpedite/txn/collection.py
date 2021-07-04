"""
Transaction Collection

TransactionCollection - a collection of all transaction in a profiling session.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from collections  import OrderedDict

class TxnSubCollection(object):
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
    return TxnSubCollection(self.name, self.cpuInfo, [], self.probes, self.topdownMetrics, self.events)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class TxnCollection(object):
  """A collection of transactions sharing a common route"""

  def __init__(self, name, cpuInfo, txnMap, probes, topdownMetrics, events, dataSource):
    self.name = name
    self.cpuInfo = cpuInfo
    for txn in txnMap.values():
      txn.finalize()
    self.txnMap = OrderedDict(sorted(txnMap.items(), key=lambda pair: pair[1].begin.tsc))
    if len(self.txnMap) != len(txnMap):
      raise Exception('failed to reorder transaction for {}'.format(name))
    self.probes = probes
    self.topdownMetrics = topdownMetrics
    self.events = events
    self.dataSource = dataSource
    self.repo = None

  def getSubCollection(self):
    """Builds an instance of transaction sub collection"""
    return TxnSubCollection(
      self.name, self.cpuInfo, list(self.txnMap.values()), self.probes, self.topdownMetrics, self.events
    )

  def isCurrent(self):
    """Returns Ture, if this collection has transactions for the current profiling session"""
    return self.repo.getCurrent() is self if self.repo else False

  def __iter__(self):
    for txn in self.txnMap.values():
      yield txn

  def __repr__(self):
    rep = ''
    for txn in self:
      rep += '{}\n'.format(txn)
    return rep

  def __eq__(self, other):
    selfDict = dict((k, val) for k, val in self.__dict__.items() if k not in ('repo', 'dataSource'))
    otherDict = dict((k, val) for k, val in other.__dict__.items() if k not in ('repo', 'dataSource'))
    return selfDict == otherDict
