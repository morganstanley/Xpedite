"""
Module to create and load benchmarks

This module implements the following features
  1. Creation of new benchmark from a profiling session
  2. Discovery of all benchmarks stored under a parent directory
  3. Logic to load benchmark info and transactions from discovered benchmarks

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import shutil
import logging
from datetime               import date
from xpedite.txn.collector  import Collector, APPINFO_FILE_NAME
from xpedite.types          import CpuInfo
from xpedite.pmu.event      import Event
from xpedite.benchmark.info import makeBenchmarkInfo, loadBenchmarkInfo
from xpedite.dependencies   import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Six)

LOGGER = logging.getLogger(__name__)

def makeBenchmark(profiles, path):
  """
  Persists profiles to the file system for future benchmarking

  :param profiles: Profile data for the benchmark
  :param path: File system path to persist the benchmark

  """
  benchmarkName = os.path.basename(path)
  if not os.path.isdir(path):
    raise Exception('Failed to make benchmark - path {} not valid'.format(path))
  txnCollection = profiles.transactionRepo.getCurrent()
  samplePath = txnCollection.dataSource.samplePath
  shutil.copytree(samplePath, os.path.join(path, os.path.basename(samplePath)))
  shutil.copyfile(txnCollection.dataSource.appInfoPath, os.path.join(path, APPINFO_FILE_NAME))
  makeBenchmarkInfo(benchmarkName, path, profiles.cpuInfo, profiles.events)

class Benchmark(object):
  """Class to load and store benchmark data"""

  def __init__(self, name, cpuInfo, path, legend, events, dataSource=None):
    self.name = name
    self.cpuInfo = cpuInfo
    self.path = path
    self.legend = legend
    self.dataSource = dataSource
    self.events = events

  def __repr__(self):
    return 'Benchmark {}: {}'.format(self.name, self.dataSource)

class BenchmarksCollector(object):
  """Collector to scan filesystem for gathering benchmarks"""

  def __init__(self, benchmarkPaths=None):
    self.benchmarkPaths = benchmarkPaths

  def gatherBenchmarks(self, count):
    """
    Gathers benchmarks from a list of paths in the file system

    :param count: Max count of benchmarks to load

    """
    benchmarks = []
    if not self.benchmarkPaths:
      return benchmarks

    for i, path in enumerate(self.benchmarkPaths):
      if os.path.isdir(path):
        info = loadBenchmarkInfo(path)
        if info:
          (benchmarkName, cpuInfo, legend, events) = info
          benchmark = Benchmark(benchmarkName, cpuInfo, path, legend, events)
          dataSource = Collector.gatherDataSource(path)
          if dataSource:
            benchmark.dataSource = dataSource
            benchmarks.append(benchmark)
        else:
          LOGGER.warn('skip processing benchmark %s. failed to load benchmark info', path)

        if len(benchmarks) >= count:
          if i + 1 < len(self.benchmarkPaths):
            LOGGER.debug('skip processing %s benchmarks. limit reached.', self.benchmarkPaths[i+1:])
          break
      else:
        LOGGER.warn('skip processing benchmark %s. failed to locate benchmark files', path)
    return benchmarks

  @staticmethod
  def loadTxns(repo, counterFilter, benchmarks, loaderFactory):
    """
    Loads transactions for a list of benchmarks

    :param repo: Transaction repo to collect transactions loaded for benchmarks
    :param counterFilter: Filter to exclude counters from loading
    :param benchmarks: List of benchmarks to be loaded
    :param loaderFactory: Factory to instantiate a loader instance

    """
    for benchmark in benchmarks:
      loader = loaderFactory(benchmark)
      collector = Collector(counterFilter)
      collector.loadDataSource(benchmark.dataSource, loader)
      repo.addBenchmark(loader.getData())
