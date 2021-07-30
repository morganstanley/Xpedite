"""
Transaction repository

This module defines a container to store transaction collection from multiple runs.
The repo is used to store profile data from multiple benchmarks along with current run.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import logging
from collections      import OrderedDict

LOGGER = logging.getLogger(__name__)

class TxnRepo(object):
  """A repository transactions from current profile session and all benchmarks"""

  def __init__(self):
    self._benchmarkCollections = OrderedDict()
    self._currentCollection = None

  def addBenchmark(self, transCollection):
    """
    Adds transaction collection for a benchmark to this repository

    :param transCollection: Transaction collection for a benchmark profile

    """
    if self._currentCollection and self._currentCollection is  transCollection:
      from xpedite.types import InvariantViloation
      raise InvariantViloation(
        'attempting to add current transaction collection {} as benchmark'.format(
          transCollection
        )
      )
    transCollection.repo = self
    self._benchmarkCollections.update({transCollection.name : transCollection})

  def addCurrent(self, transCollection):
    """
    Adds transaction collection for current profile session to this repository

    :param transCollection: Transaction collection for current profile session

    """
    if self._currentCollection:
      raise Exception(
        'Attempt to register multiple current collections - Repository already has collection "{}" '
        'marked current'.format(self._currentCollection)
      )
    self._currentCollection = transCollection
    transCollection.repo = self

  def getCurrent(self):
    """Returns transaction collection for the current profile session"""
    return self._currentCollection

  def hasBenchmarks(self):
    """Returns True, If the repository has benchmark transactions"""
    return len(self._benchmarkCollections) > 0

  def getBenchmarks(self):
    """Returns transaction collections for, all the benchmarks in this repo"""
    return self._benchmarkCollections

  def getBenchmark(self, name):
    """
    Returns transaction collection for the given benchmark

    :param name: name of the benchmark

    """
    return self._benchmarkCollections[name]

  def getTxnCollections(self):
    """Returns transaction collections for current profile session and benchmarks"""
    return [self._currentCollection] + list(self._benchmarkCollections.values())

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

def loaderFactory(loaderType, benchmark, probes, benchmarkProbes, topdownCache, topdownMetrics):
  """
  Builds a loader instance for construction of transactions from counters

  :param loaderType: Type of the loader instance to be instantiated
  :param benchmark: Handle to instance of benchmark object
  :type benchmark: xpedite.benchmark.Benchmark
  :param probes: List of probes enabled for the profile session
  :param benchmarkProbes: List of probes enabled for the benchmark session
  :param topdownCache: A cache of supported topdown hierarchies
  :param topdownMetrics: Top down metrics to be computed

  """
  loaderProbes = probes
  benchmarkTopdownMetrics = None
  if benchmarkProbes and benchmark.name in benchmarkProbes:
    loaderProbes = benchmarkProbes[benchmark.name]
    LOGGER.warn('overriding probes for benchmark run \'%s\'', benchmark.name)
  if benchmark.events and topdownMetrics:
    from xpedite.pmu.event import TopdownMetrics
    benchmarkTopdownMetrics = TopdownMetrics()
    topdown = topdownCache.get(benchmark.cpuInfo.cpuId)
    eventSet = set(event.uarchName for event in benchmark.events)

    def canAdd(node):
      """Callable to detect, if a benchmark has pmu data for a required events"""
      for event in node.events:
        if event.name not in eventSet:
          return False
      return True

    for topdownKey in topdownMetrics.topdownKeys():
      benchmarkTopdownMetrics.add(topdown, topdownKey, canAdd)
  return loaderType(benchmark.name, benchmark.cpuInfo, loaderProbes, benchmarkTopdownMetrics, benchmark.events)

class TxnRepoFactory(object):
  """Factory to build a repository of transactions"""

  @staticmethod
  def buildTxnRepo(app, cpuInfo, probes, topdownCache, topdownMetrics,
    events, benchmarkProbes, benchmarkPaths):
    """
    Builds a repository of transactions for current profile session and benchmarks

    :param app: An instance of xpedite app, to interact with target application
    :param cpuInfo: Cpu info of the host running target app
    :param probes: List of probes enabled for the profile session
    :param topdownCache: A cache of supported topdown hierarchies
    :param topdownMetrics: Top down metrics to be computed
    :param events: PMU events collected for the profiling session
    :param benchmarkProbes: List of probes enabled for the benchmark session
    :param benchmarkPaths: List of stored reports from previous runs, for benchmarking

    """
    from xpedite.txn.collector        import Collector
    from xpedite.benchmark            import BenchmarksCollector
    from xpedite.txn.loader           import BoundedTxnLoader
    from xpedite.txn.filter           import TrivialCounterFilter
    from xpedite.analytics            import CURRENT_RUN
    from xpedite.util                 import timeAction
    counterFilter = TrivialCounterFilter()
    collector = Collector(counterFilter)

    loaderType = BoundedTxnLoader
    loader = loaderType(CURRENT_RUN, cpuInfo, probes, topdownMetrics, events)

    timeAction('gathering counters', lambda: collector.gatherCounters(app, loader))
    currentTxns = loader.getData()

    if not currentTxns:
      if loader.processedCounterCount:
        msg = 'failed to load transactions. recheck routes specified in your profile info'
        LOGGER.error(msg)
        raise Exception(msg)
      msg = 'failed to load transactions. It appears the app hit any of the activated probes'
      LOGGER.error(msg)
      raise Exception(msg)

    repo = TxnRepo()
    repo.addCurrent(currentTxns)

    if benchmarkPaths:
      benchmarksCollector = BenchmarksCollector(benchmarkPaths)
      benchmarksCollector.loadTxns(
        repo, counterFilter, benchmarksCollector.gatherBenchmarks(10), loaderFactory=lambda benchmark: loaderFactory(
          loaderType, benchmark, probes, benchmarkProbes, topdownCache, topdownMetrics
        )
      )
    return repo
