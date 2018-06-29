"""
Xpedite Analytics package

This package includes
  1. Logic for grouping data from related probes, to build transactions
  2. Classify and aggreate transactions based on route (control flow)
  3. Build timelines and duration series from aggregated transactions
  4. Logic to conflate transactions from mulitiple profiles

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.dependencies                import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Numpy)
import sys
from xpedite.analytics.aggregator        import TransactionAggregator, RouteAggregator, RouteConflatingAggregator
from xpedite.util                        import timeAction
from xpedite.analytics.timeline          import buildTimelineStats
from xpedite.analytics.treeCollections   import TreeCollectionFactory
import logging

LOGGER = logging.getLogger(__name__)

CURRENT_RUN = 'current run'

class Analytics(object):

  """Analytics logic to build transactions for current profile session and bechmarks"""

  @staticmethod
  def buildElapsedTimeBundles(transactionCollections, classifier):
    """
    Builds elapsed timestamp counters for each of the categories in given transaction collections

    :param repo: List of transaction collections from current profile session and benchmarks
    :param classifier: Predicate to classify transactions into different categories

    """
    elapsedTscBundles = {}
    categorySet = set()
    for i, txnCollection in enumerate(transactionCollections):
      txnSubCollection = txnCollection.getSubCollection()
      probes = txnCollection.probes
      elapsedTscMap = timeAction(
        'aggregating time stamp counters per transaction',
        lambda txnsc=txnSubCollection, txnCollection=txnCollection: TransactionAggregator.groupElapsedTime(
          txnsc, txnCollection.cpuInfo, classifier=classifier
        )
      )
      if len(elapsedTscMap) > 0:
        for category, elapsedTscList in elapsedTscMap.iteritems():
          if category in elapsedTscBundles:
            elapsedTscBundles[category].append(elapsedTscList)
          else:
            if i == 0:
              elapsedTscBundles.update({category:[elapsedTscList]})
            else:
              if category not in categorySet:
                categorySet.add(category)
                LOGGER.warn('current run missing trasactions for category "%s"', category)
      else:
        scopeList = ', '.join([probe.getCanonicalName() for probe in probes if not probe.isAnonymous])
        errMsg = (
          """{}({}) doesn\'t have any transaction (#{} loaded) with probes\n\t[{}]
          Did you generate any of these transactions ?"""
        )
        errMsg = errMsg.format(' benchmark ' if i > 0 else ' ', txnCollection.name
          , len(txnSubCollection), scopeList)
        LOGGER.error(errMsg)
        if i == 0:
          raise Exception('report generation failed for current run. counters not available')
    return elapsedTscBundles

  @staticmethod
  def buildTransactionTree(transactionRepo, transactionClassifier):
    """
    Builds Transaction tree collections for current profile sesssion and benchmarks

    :param repo: Repository of transaction collections from current profile session and benchmarks
    :param transactionClassifier: Predicate to classify transactions into different categories

    """
    mustHaveProbes = None
    treeClassifiers = [
      lambda txnSubCollection, ancestry: TransactionAggregator.groupTransactions(
        txnSubCollection, classifier=transactionClassifier, mustHaveProbes=mustHaveProbes
      ),
      lambda txnSubCollection, ancestry: RouteAggregator.aggregateTransactionsByRoutes(txnSubCollection)
    ]
    transactionTree = TreeCollectionFactory.buildTreeCollection(
      transactionRepo.getCurrent().name, transactionRepo.getCurrent().getSubCollection(), treeClassifiers
    )

    treeClassifiers[1] = RouteConflatingAggregator(transactionTree).aggregateTransactionsByRoutes
    benchmarkCompositeTree = TreeCollectionFactory.buildCompositeTreeCollection(
      {name : collection.getSubCollection() for name, collection in transactionRepo.getBenchmarks().iteritems()},
      treeClassifiers
    )
    return(transactionTree, benchmarkCompositeTree)

  @staticmethod
  def computeStats(transactionRepo, category, route, probes, txnSubCollection, benchmarkTransactionsMap):
    """
    Computes timeline statistics for transactions from current profile session and all benchmarks

    :param transactionRepo: Repository of transactions from current profile session and benchmarks
    :param category: Category of transactions in the given subcollection
    :param route: Route taken by the transactions in subcollection
    :param probes: List of probes enabled during the current profile session
    :param txnSubCollection: Subcollection of transactions to compute statistics for
    :param benchmarkTransactionsMap: Map of transaction subcollections in benchmarks

    """
    timelineStats = buildTimelineStats(category, route, probes, txnSubCollection)
    benchmarkTimelineStats = {}
    if benchmarkTransactionsMap:
      benchmarkTimelineStats = {
        txnSubCollection.name: buildTimelineStats(
          category, route, probes, txnSubCollection) for txnSubCollection in benchmarkTransactionsMap.values()
      }
    elif transactionRepo.hasBenchmarks():
      LOGGER.warn('[benchmarks missing category/route]')
    return timelineStats, benchmarkTimelineStats

  @staticmethod
  def filterTransactions(repo, txnFilter):
    """
    Filters transactions using the given callable (txnFilter)

    :param repo: Repository of transactions from current profiling session and benchmarks
    :type repo: xpedite.transaction.TransactionRepo
    :param txnFilter: filter to be excluded transaction from reporting
    :type txnFilter: callable

    """
    totalFilteredCount = 0
    for txnCollection in repo.getTransactionCollections():
      transactionsMap = txnCollection.transactionsMap
      filteredCount = 0
      unfilteredCount = len(transactionsMap)
      for tid, txn in transactionsMap.iteritems():
        if not txnFilter(txnCollection.name, txn):
          del transactionsMap[tid]
          filteredCount += 1
      if filteredCount:
        LOGGER.debug('filtering txns from \"%s\" - removed %d out of %d',
          txnCollection.name, filteredCount, unfilteredCount
        )
      totalFilteredCount += filteredCount
    return totalFilteredCount
