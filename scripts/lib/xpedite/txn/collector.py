"""
Collector to collect and process profile data
This module is used to load counter data from xpedite text format sample files.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import time
import logging
from xpedite.txn.extractor      import Extractor

LOGGER = logging.getLogger(__name__)

class Collector(Extractor):
  """Parses sample files to gather time and pmu counters"""

  def __init__(self, counterFilter):
    """
    Constructs an instance of collector

    :param counterFilter: a filter to exclude compromised or unused counters
    :type counterFilter: xpedite.filter.TrivialCounterFilter

    """
    Extractor.__init__(self, counterFilter)

  @staticmethod
  def formatPath(path, maxChars):
    """
    Trims path to at most max characters

    :param path: file system path
    :param maxChars: maximum allowed characters for the path

    """
    return path if len(path) < maxChars else '...' + path[-maxChars:]

  def loadDataSource(self, dataSource, loader):
    """
    Loads counters for the given dataSource

    :param dataSource: List of data sources with profile data
    :param loader: Loader implementation to build transactions from counters

    """
    from xpedite.profiler.appInfo import AppInfo
    loader.beginCollection(dataSource)
    appInfo = AppInfo(dataSource.appInfoPath)
    appInfo.load()
    self.loadSamples(loader, appInfo.probes, dataSource)
    loader.endCollection()

  def loadSamples(self, loader, probes, dataSource):
    """
    Loads counters for a profile session from csv sample files

    :param loader: Loader to build transactions out of the counters
    :param probes: A list of probes associated with samples in a file
    :param path: Path to sample file for a thread with counter data in csv format

    """
    recordCount = 0
    for sampleFile in dataSource.files:
      loader.beginLoad(sampleFile.threadId, sampleFile.tlsAddr)
      recordCount += self.loadCounters(sampleFile.threadId, loader, probes, sampleFile.path)
      loader.endLoad()
    self.logCounterFilterReport()
    return recordCount

  def loadCounters(self, threadId, loader, probes, path):
    """
    Loads counters for a thread from csv sample files

    :param loader: Loader to build transactions out of the counters
    :param threadId: Id of the thread, that captured the counters
    :type threadId: str
    :param probes: Map of probes instrumented in target application
    :param path: Path to file with counters to be loaded
    :type path: str

    """
    LOGGER.info('loading report file %s -> ', self.formatPath(path, 70))
    begin = time.time()
    with open(path) as fileHandle:
      recordCount = 0
      for record in fileHandle:
        if recordCount > 0:
          self.loadCounter(threadId, loader, probes, record)
        recordCount += 1
      elapsed = time.time() - begin
      LOGGER.completed('%d records | %d txns loaded in %0.2f sec.', recordCount-1,
        loader.getCount(), elapsed)
      return recordCount

  def gatherCounters(self, app, loader):
    """
    Gathers time and pmu counters from sample files for a profile session

    :param app: Handle to the instance of the xpedite app
    :param loader: Loader to build transactions out of the counters

    """
    if app.dataSource:
      return self.loadDataSource(app.dataSource, loader)
    return Extractor.gatherCounters(self, app, loader)
