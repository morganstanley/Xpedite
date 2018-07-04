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
import ConfigParser
from datetime               import date
from xpedite.txn.collector  import Collector
from xpedite.types          import DataSource, CpuInfo
from xpedite.pmu.event      import Event

LOGGER = logging.getLogger(__name__)

BENCHMARK_DIR_NAME = 'benchmark'
BENCHMARK_FILE_NAME = '.benchmark'
BENCHMARK_APPINFO_FILE_NAME = 'appinfo.txt'
BENCHMARK_SECTION = 'info'
BENCHMARK_NAME_KEY = 'name'
BENCHMARK_LEGEND_KEY = 'legend'

BENCHMARK_CPU_INFO_SECTION = 'cpuInfo'
BENCHMARK_CPU_ID_KEY = 'id'
BENCHMARK_CPU_FREQUENCY_KEY = 'frequency'

BENCHMARK_PMC_SECTION = 'pmc'
BENCHMARK_PMC_COUNTER_COUNT = 'counterCount'
BENCHMARK_PMC_COUNTER = 'counter#{}'

def makeBenchmarkInfo(benchmarkName, profiles, path):
  """
  Creates an info file with benchmark details in human readable format

  :param benchmarkName: Name of the benchmark
  :param path: Path of the benchmark info file
  :param profiles: Profile data for the benchmark

  """
  path = os.path.join(path, BENCHMARK_FILE_NAME)
  config = ConfigParser.RawConfigParser()
  config.add_section(BENCHMARK_SECTION)
  config.set(BENCHMARK_SECTION, BENCHMARK_NAME_KEY, benchmarkName)
  legend = '{} run at {}'.format(benchmarkName, str(date.today()))
  config.set(BENCHMARK_SECTION, BENCHMARK_LEGEND_KEY, legend)

  cpuInfo = profiles.cpuInfo
  config.add_section(BENCHMARK_CPU_INFO_SECTION)
  config.set(BENCHMARK_CPU_INFO_SECTION, BENCHMARK_CPU_ID_KEY, cpuInfo.cpuId)
  config.set(BENCHMARK_CPU_INFO_SECTION, BENCHMARK_CPU_FREQUENCY_KEY, cpuInfo.frequency)

  if profiles.events:
    config.add_section(BENCHMARK_PMC_SECTION)
    config.set(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER_COUNT, len(profiles.events))
    for i, event in enumerate(profiles.events):
      value = '{},{},{},{}'.format(event.name, event.uarchName, event.user, event.kernel)
      config.set(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER.format(i), value)
  with open(path, 'w') as configfile:
    config.write(configfile)

def makeBenchmark(profiles, path):
  """
  Persists profiles to the file system for future benchmarking

  :param profiles: Profile data for the benchmark
  :param path: File system path to persist the benchmark

  """
  benchmarkName = os.path.basename(path)
  path = os.path.join(path, BENCHMARK_DIR_NAME)
  if os.path.exists(path):
    raise Exception('Failed to make benchmark - path {} already exists'.format(path))
  txnCollection = profiles.transactionRepo.getCurrent()
  for dataSource in txnCollection.dataSources:
    shutil.copytree(dataSource.path, os.path.join(path, os.path.basename(dataSource.path)))
    shutil.copyfile(dataSource.appInfoPath, os.path.join(path, BENCHMARK_APPINFO_FILE_NAME))
  makeBenchmarkInfo(benchmarkName, profiles, path)

class Benchmark(object):
  """Class to load and store benchmark data"""

  def __init__(self, name, cpuInfo, path, legend, events, dataSources=None):
    self.name = name
    self.cpuInfo = cpuInfo
    self.path = path
    self.legend = legend
    self.dataSources = dataSources
    self.events = events

  def __repr__(self):
    return 'Benchmark {}: {}'.format(self.name, self.dataSources)

class BenchmarksCollector(object):
  """Collector to scan filesystem for gathering benchmarks"""

  def __init__(self, benchmarkPaths=None):
    self.benchmarkPaths = benchmarkPaths

  @staticmethod
  def loadBenchmarkInfo(path):
    """
    Loads info about benchmark from file system

    :param path: path of the benchmark info file

    """
    configParser = ConfigParser.RawConfigParser()
    fileName = os.path.join(path, BENCHMARK_FILE_NAME)
    if os.path.exists(fileName):
      configParser.read(fileName)
      benchmarkName = configParser.get(BENCHMARK_SECTION, BENCHMARK_NAME_KEY)
      legend = configParser.get(BENCHMARK_SECTION, BENCHMARK_LEGEND_KEY)

      if not configParser.has_section(BENCHMARK_CPU_INFO_SECTION):
        LOGGER.warn('failed to load benchmark %s - cpu info missing', benchmarkName)
        return None
      cpuId = configParser.get(BENCHMARK_CPU_INFO_SECTION, BENCHMARK_CPU_ID_KEY)
      cpuFrequency = configParser.get(BENCHMARK_CPU_INFO_SECTION, BENCHMARK_CPU_FREQUENCY_KEY)
      cpuInfo = CpuInfo(cpuId, int(cpuFrequency))
      events = None
      if configParser.has_option(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER_COUNT):
        counterCount = configParser.getint(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER_COUNT)
        events = []
        for i in range(counterCount):
          eventStr = configParser.get(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER.format(i))
          eventFields = eventStr.split(',')
          events.append(Event(eventFields[0], eventFields[1], bool(eventFields[2]), bool(eventFields[3])))
      return Benchmark(benchmarkName, cpuInfo, path, legend, events)
    return None


  def gatherBenchmarks(self, count):
    """
    Gathers benchmarks from a list of paths in the file system

    :param count: Max count of benchmarks to load

    """
    benchmarks = []
    if not self.benchmarkPaths:
      return benchmarks

    for i, path in enumerate(self.benchmarkPaths):
      benchmarkPath = os.path.join(path, BENCHMARK_DIR_NAME)
      if os.path.isdir(benchmarkPath):
        benchmark = self.loadBenchmarkInfo(benchmarkPath)
        if benchmark:
          dataSources = self.gatherDataSources(benchmarkPath)
          if dataSources:
            benchmark.dataSources = dataSources
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
  def gatherDataSources(path):
    """
    Gathers the list of profile data files for a benchmark

    :param path: path to benchmark directory

    """
    dataSources = []
    for runId in os.listdir(path):
      reportPath = os.path.join(path, runId)
      appInfoPath = os.path.join(path, BENCHMARK_APPINFO_FILE_NAME)
      if os.path.isdir(reportPath) and os.path.isfile(appInfoPath):
        dataSources.append(DataSource(appInfoPath, reportPath))
    return dataSources

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
      collector.loadDataSources(benchmark.dataSources, loader)
      repo.addBenchmark(loader.getData())
