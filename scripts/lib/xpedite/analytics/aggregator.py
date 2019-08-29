"""
Module to aggregate transactions, routes and counters
  1. RouteAggregator - Conflates long routes to short ones
  2. TxnAggregator - Classifies transaction based on a predicate and aggregates elasped cycles

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from collections                import OrderedDict
from xpedite.txn.classifier     import DefaultClassifier

def txnSubCollectionFactory(txnSubCollection, txn):
  """
  Builds a new transaction subcollection with meta data matching given source collection

  :param txnSubCollection: Source collection to clone meta data for the new subcollection
  :param txn: transaction to be added to the new subcollection

  """
  subCollection = txnSubCollection.cloneMetaData()
  subCollection.append(txn)
  return subCollection

def addTxn(routeMap, txnSubCollection, route, txn):
  """
  Adds transaction to a transaction subcollection with matching route

  :param routeMap: Map of routes taken by all the transaction in the profile
  :param route: Route taken by the given transaction
  :param txnSubCollection: Transaction subcollection to clone meta data from
  :param txn: Transaction to be added

  """
  if route in routeMap:
    routeMap[route].append(txn)
  else:
    routeMap.update({route : txnSubCollectionFactory(txnSubCollection, txn)})

class RouteAggregator(object):
  """Aggregates transactions based their respective routes"""

  @staticmethod
  def aggregateTxnsByRoutes(txnSubCollection):
    """
    Aggregates transactions in a given source subcollection, to multiple subcollections
    based on their respective routes

    :param txnSubCollection: Transaction subcollection to be aggregated

    """
    routeMap = OrderedDict()
    for txn in txnSubCollection:
      addTxn(routeMap, txnSubCollection, txn.route, txn)
    return routeMap

class RouteConflatingAggregator(object):
  """Aggregates transactions to a set of conflatable source routes"""

  def __init__(self, srcTree):
    self.srcTree = srcTree

  def aggregateTxnsByRoutes(self, txnSubCollection, ancestry):
    """
    Aggregates transactions in a given subcollection to child collections
    in an ancestry node with conflatable routes

    :param txnSubCollection: Transaction subcollection to be aggregated
    :param ancestry: A node in the tree collection

    """
    from xpedite.types.route import conflateRoutes
    srcRouteMap = self.srcTree.getChildren(ancestry)
    routes = srcRouteMap.keys() if srcRouteMap else []
    routeMap = {}
    for txn in txnSubCollection:
      for dstRoute in routes:
        if conflateRoutes(txn.route, dstRoute):
          addTxn(routeMap, txnSubCollection, dstRoute, txn)
    return routeMap

class TxnAggregator(object):
  """Aggregates transaction by categories"""

  begin = 0
  end = 1

  @staticmethod
  def _addOrUpdateContainer(container, subCollectionFactory, classifier, key, value):
    """
    Adds transaction to a transaction subcollection with matching category

    :param container: Container with all categories of aggreagated values
    :param subCollectionFactory: Callable used to build an instance of subcollection
    :param classifier: Predicate to classify transactions into different categories
    :param key: Key used for classification
    :param value: Value to be aggregated

    """
    category = classifier.classify(key)
    if category in container:
      container[category].append(value)
    else:
      container.update({category : subCollectionFactory(value)})

  @staticmethod
  def groupElapsedTscByScope(txnSubCollection, beginProbe, endProbe, classifier=DefaultClassifier()):
    """
    Aggregates elapsed tsc values by category

    :param txnSubCollection: Transaction subcollection to be aggregated
    :param beginProbe: Begin probe used for elapsed tsc computation
    :param endProbe: End probe used for elapsed tsc computation
    :param classifier: Predicate to classify transactions into different categories

    """
    elapsedTscGroup = {}
    for txn in txnSubCollection:
      if txn.hasProbes([beginProbe, endProbe]):
        beginCounter = txn.getCounterForProbe(beginProbe)
        endCounter = txn.getCounterForProbe(endProbe)
        TxnAggregator._addOrUpdateContainer(
          elapsedTscGroup, lambda v: [v], classifier, txn,
          endCounter.tsc - beginCounter.tsc
        )
    return elapsedTscGroup

  @staticmethod
  def groupElapsedTime(txnSubCollection, cpuInfo, classifier=DefaultClassifier()):
    """
    Aggregates elapsed time by category

    This method computes elapsed wall time for each transaction in the subcollection
    and aggregates computed duration by its soruce transaction's category

    :param txnSubCollection: Transaction subcollection to be aggregated
    :param classifier: Predicate to classify transactions into different categories
    :param cpuInfo: Cpu info to convert cycles to duration (micro seconds)

    """
    elapsedTscGroup = {}
    for txn in txnSubCollection:
      if txn:
        time = cpuInfo.convertCyclesToTime(txn.getElapsedTsc())
        TxnAggregator._addOrUpdateContainer(elapsedTscGroup, lambda v: [v], classifier, txn, time)
    return elapsedTscGroup

  @staticmethod
  def groupTxns(txnSubCollection, classifier=DefaultClassifier(), mustHaveProbes=None):
    """
    Aggregates transactions by their resepective categories

    :param txnSubCollection: Transaction subcollection to be aggregated
    :param classifier: Predicate to classify transactions into different categories
    :param mustHaveProbes: Probes used to exclude transaction from aggregation

    """
    groupMap = {}

    # classifiy the counter breakups into categories
    for txn in txnSubCollection:
      if not mustHaveProbes or txn.hasProbes(mustHaveProbes):
        TxnAggregator._addOrUpdateContainer(
          groupMap, lambda t: txnSubCollectionFactory(txnSubCollection, t),
          classifier, txn, txn
        )
    return groupMap
