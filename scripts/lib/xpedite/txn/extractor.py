"""
Extractor to extract and collect xpedite samples data

This module is used to load counter data from xpedite binary sample files.
A decoder is used to open and extract timing and pmc data captured by
probes in the target application.
Each such decoded record is inturn used to construct a Counter object for
transaction building.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import sys
import os
import re
import time
import logging
from xpedite.types      import Counter, DataSource
from xpedite.util       import makeLogPath, mkdir

LOGGER = logging.getLogger(__name__)

class Extractor(object):
  """Parses sample files to load counters for the current profile session"""

  def __init__(self, counterFilter):
    """
    Constructs a new instance of extractor

    :param counterFilter: Filter to exclude out compromised or unused counters
    :type counterFilter: xpedite.filter.TrivialCounterFilter

    """
    self.binaryReportFilePattern = re.compile(r'[^\d]*(\d+)-(\d+)-([0-9a-fA-F]+)\.data')
    self.counterFilter = counterFilter
    self.orphanedSamplesCount = 0

  def gatherCounters(self, app, loader):
    """
    Gathers time and pmu counters from sample files for the current profile session

    :param app: Handle to the instance of the xpedite app
    :type app: xpedite.profiler.environment.XpediteApp
    :param loader: Loader to build transactions out of the counters

    """
    pattern = app.sampleFilePattern()
    LOGGER.info('scanning for samples files matching - %s', pattern)
    filePaths = app.gatherFiles(pattern)

    samplePath = makeLogPath('{}/{}'.format(app.name, app.runId))
    dataSource = DataSource(app.appInfoPath, samplePath)
    loader.beginCollection(dataSource)

    for filePath in filePaths:
      (threadId, tlsAddr) = self.extractThreadInfo(filePath)
      if not threadId or not tlsAddr:
        raise Exception('failed to extract thread info for file {}'.format(filePath))
      LOGGER.info('loading counters for thread %s from file %s -> ', threadId, filePath)

      iterBegin = begin = time.time()
      loader.beginLoad(threadId, tlsAddr)

      moduleDirPath = os.path.dirname(os.path.abspath(__file__))
      bindingModulePath = '{}/../../../../install/lib'.format(moduleDirPath)
      sys.path.append(bindingModulePath)
      from xpediteBindings    import SamplesLoader
      samplesLoader = SamplesLoader(filePath)
      recordCount = 0
      for sample in samplesLoader:
        self.loadSample(threadId, loader, app.probes, sample)
        elapsed = time.time() - iterBegin
        if elapsed >= 5:
          LOGGER.completed('\tprocessed %d counters | ', recordCount-1)
          iterBegin = time.time()
        recordCount += 1
      loader.endLoad()
      elapsed = time.time() - begin
      self.logCounterFilterReport()
      if self.orphanedSamplesCount:
        LOGGER.warn('detected mismatch in binary vs app info - %d counters ignored', self.orphanedSamplesCount)
      LOGGER.completed('%d records | %d txns loaded in %0.2f sec.', recordCount-1, loader.getCount(), elapsed)
    if loader.isCompromised() or loader.getTxnCount() <= 0:
      LOGGER.warn(loader.report())
    elif loader.isNotAccounted():
      LOGGER.debug(loader.report())
    loader.endCollection()

  def loadSample(self, threadId, loader, probes, sample):
    """
    Loads time and pmu counters from xpedite sample objects

    :param threadId: Id of thread collecting the samples
    :param loader: loader to build transactions out of the counters
    :param probes: Map of probes instrumented in target application
    :param sample: An object with binding to underlying C++ Sample object

    """
    addr = hex(sample.returnSite())
    if addr not in probes:
      self.orphanedSamplesCount += 1
      return None
    data = sample.dataStr() if sample.hasData() else ''
    counter = Counter(threadId, probes[addr], data, sample.tsc())
    if sample.hasPmc():
      for i in range(sample.pmcCount()):
        counter.addPmc(sample.pmc(i))
    if self.counterFilter.canLoad(counter):
      loader.loadCounter(counter)
    return counter

  MIN_FIELD_COUNT = 2
  INDEX_TSC = 0
  INDEX_ADDR = 1
  INDEX_DATA = 2
  INDEX_PMC = 3

  def loadCounter(self, threadId, loader, probes, record):
    """
    Loads time and pmu counters from the given record

    :param threadId: Id of thread collecting the samples
    :param loader: loader to build transactions out of the counters
    :param probes: Map of probes instrumented in target application
    :param record: A sample record in csv format

    """
    fields = record.split(',')
    if len(fields) < self.MIN_FIELD_COUNT:
      raise Exception('detected record with < {} fields - \nrecord: "{}"\n'.format(self.MIN_FIELD_COUNT, record))
    addr = fields[self.INDEX_ADDR]
    if addr not in probes:
      self.orphanedSamplesCount += 1
      return None
    data = fields[self.INDEX_DATA]
    tsc = int(fields[self.INDEX_TSC], 16)

    counter = Counter(threadId, probes[addr], data, tsc)
    if len(fields) > self.MIN_FIELD_COUNT:
      for pmc in fields[self.MIN_FIELD_COUNT+1:]:
        counter.addPmc(int(pmc))
    if self.counterFilter.canLoad(counter):
      loader.loadCounter(counter)
    return counter

  @staticmethod
  def openInflateFile(dataSourcePath, threadId, tlsAddr):
    """
    Creates a new data source file for the given thread

    :param dataSourcePath: Path of the data source directory
    :param threadId: Id of thread collecting the samples
    :param tlsAddr: Address of thread local storage of thread collecting the samples

    """
    path = os.path.join(dataSourcePath, '{}-{}'.format(threadId, tlsAddr))
    mkdir(path)
    filePath = os.path.join(path, 'samples-0000.csv')
    #pylint: disable=consider-using-with
    return open(filePath, 'w')

  def extractThreadInfo(self, samplesFile):
    """
    Extracts thread id/thread local storage address from name of the samples file

    :param samplesFile: Name of the samples file

    """
    match = self.binaryReportFilePattern.findall(samplesFile)
    if match and len(match[0]) > 2:
      return (match[0][1], match[0][2])
    return (None, None)

  def logCounterFilterReport(self):
    """Logs statistics on the number of filtered counters"""
    report = self.counterFilter.report()
    if report:
      if self.counterFilter.extraneousCounters > 0:
        LOGGER.error(report)
      else:
        LOGGER.error(report)
    self.counterFilter.reset()
