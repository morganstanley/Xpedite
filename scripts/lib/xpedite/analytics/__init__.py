"""
Xpedite Analytics package

This package includes
  1. Logic for grouping data from related probes, to build transactions
  2. Classify and aggreate transactions based on route (control flow)
  3. Build timelines and duration series from aggregated transactions
  4. Logic to conflate transactions from mulitiple profiles

Author: Manikandan Dhamodharan, Morgan Stanley
"""
import sys
import time
import logging
from xpedite.util                        import timeAction
from xpedite.types.containers            import ProbeMap

from xpedite.dependencies                import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Numpy)
from xpedite.analytics.aggregator        import TxnAggregator, RouteAggregator, RouteConflatingAggregator # pylint: disable=wrong-import-position
from xpedite.analytics.timeline          import buildTimelineStats # pylint: disable=wrong-import-position
from xpedite.analytics.treeCollections   import TreeCollectionFactory # pylint: disable=wrong-import-position

LOGGER = logging.getLogger(__name__)

CURRENT_RUN = 'current run'

class Analytics(object):

  """Analytics logic to build transactions for current profile session and bechmarks"""

  @staticmethod
  def buildElapsedTimeBundles(txnCollections, classifier):
    """
    Builds elapsed timestamp counters for each of the categories in given transaction collections

    :param repo: List of transaction collections from current profile session and benchmarks
    :param classifier: Predicate to classify transactions into different categories

    """
    elapsedTscBundles = {}
    categorySet = set()
    for i, txnCollection in enumerate(txnCollections):
      txnSubCollection = txnCollection.getSubCollection()
      probes = txnCollection.probes
      elapsedTscMap = timeAction(
        'aggregating time stamp counters per transaction',
        lambda txnsc=txnSubCollection, txnCollection=txnCollection: TxnAggregator.groupElapsedTime(
          txnsc, txnCollection.cpuInfo, classifier=classifier
        )
      )
      if elapsedTscMap:
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
  def buildTxnTree(txnRepo, txnClassifier):
    """
    Builds Transaction tree collections for current profile sesssion and benchmarks

    :param repo: Repository of transaction collections from current profile session and benchmarks
    :param txnClassifier: Predicate to classify transactions into different categories

    """
    mustHaveProbes = None
    treeClassifiers = [
      lambda txnSubCollection, ancestry: TxnAggregator.groupTxns(
        txnSubCollection, classifier=txnClassifier, mustHaveProbes=mustHaveProbes
      ),
      lambda txnSubCollection, ancestry: RouteAggregator.aggregateTxnsByRoutes(txnSubCollection)
    ]
    txnTree = TreeCollectionFactory.buildTreeCollection(
      txnRepo.getCurrent().name, txnRepo.getCurrent().getSubCollection(), treeClassifiers
    )

    treeClassifiers[1] = RouteConflatingAggregator(txnTree).aggregateTxnsByRoutes
    benchmarkCompositeTree = TreeCollectionFactory.buildCompositeTreeCollection(
      {name : collection.getSubCollection() for name, collection in txnRepo.getBenchmarks().iteritems()},
      treeClassifiers
    )
    return(txnTree, benchmarkCompositeTree)

  @staticmethod
  def computeStats(txnRepo, category, route, probes, txnSubCollection, benchmarkTxnsMap):
    """
    Computes timeline statistics for transactions from current profile session and all benchmarks

    :param txnRepo: Repository of transactions from current profile session and benchmarks
    :param category: Category of transactions in the given subcollection
    :param route: Route taken by the transactions in subcollection
    :param probes: List of probes enabled during the current profile session
    :param txnSubCollection: Subcollection of transactions to compute statistics for
    :param benchmarkTxnsMap: Map of transaction subcollections in benchmarks

    """
    timelineStats = buildTimelineStats(category, route, probes, txnSubCollection)
    benchmarkTimelineStats = {}
    if benchmarkTxnsMap:
      benchmarkTimelineStats = {
        txnSubCollection.name: buildTimelineStats(
          category, route, probes, txnSubCollection) for txnSubCollection in benchmarkTxnsMap.values()
      }
    elif txnRepo.hasBenchmarks():
      LOGGER.warn('[benchmarks missing category/route]')
    return timelineStats, benchmarkTimelineStats

  def generateProfiles(self, name, txnRepo, classifier):
    """
    Generates profiles for the current profile session

    :param txnRepo: Repository of loaded transactions
    :param classifier: Predicate to classify transactions into different categories

    """
    from xpedite.profiler.profile import Profiles, Profile
    txnTree, benchmarkCompositeTree = self.buildTxnTree(txnRepo, classifier)
    profiles = Profiles(txnRepo)

    for category, categoryNode in txnTree.getChildren().iteritems():
      i = 1
      for route, txnNode in categoryNode.children.iteritems():
        routeName = ' [route - {}]'.format(i) if len(categoryNode.children) > 1 else ''
        profileName = '{} - {}{}'.format(name, category, routeName)
        begin = time.time()
        LOGGER.info('generating profile %s (txns - %d) -> ', profileName, len(txnNode.collection))

        benchmarkTxnsMap = benchmarkCompositeTree.getCollectionMap([category, route])
        reportProbes = self.mapReportProbes(route, txnRepo.getCurrent().probes)
        timelineStats, benchmarkTimelineStats = self.computeStats(
          txnRepo, category, route, reportProbes, txnNode.collection, benchmarkTxnsMap
        )
        profiles.addProfile(Profile(profileName, timelineStats, benchmarkTimelineStats))
        elapsed = time.time() - begin
        LOGGER.completed('completed in %0.2f sec.', elapsed)
        i += 1
    return profiles

  @staticmethod
  def mapReportProbes(route, userProbes):
    """
    Creates probes with human friendly name for reporting

    :param userProbes: List of probes enabled for a profiling session

    """
    reportProbes = []
    userProbeMap = ProbeMap(userProbes)
    for probe in route.probes:
      if probe in userProbeMap:
        reportProbes.append(userProbeMap[probe])
      else:
        reportProbes.append(probe)
    return reportProbes

  @staticmethod
  def filterTxns(repo, txnFilter):
    """
    Filters transactions using the given callable (txnFilter)

    :param repo: Repository of transactions from current profiling session and benchmarks
    :type repo: xpedite.txn.repo.TxnRepo
    :param txnFilter: filter to be excluded transaction from reporting
    :type txnFilter: callable

    """
    totalFilteredCount = 0
    for txnCollection in repo.getTxnCollections():
      txnMap = txnCollection.txnMap
      filteredCount = 0
      unfilteredCount = len(txnMap)
      for tid, txn in txnMap.iteritems():
        if not txnFilter(txnCollection.name, txn):
          del txnMap[tid]
          filteredCount += 1
      if filteredCount:
        LOGGER.debug('filtering txns from \"%s\" - removed %d out of %d',
          txnCollection.name, filteredCount, unfilteredCount
        )
      totalFilteredCount += filteredCount
    return totalFilteredCount
