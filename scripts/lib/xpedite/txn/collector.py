"""
Collector to collect and process profile data
This module is used to load counter data from xpedite text format sample files.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import re
import time
import fnmatch
import logging
from xpedite.txn.extractor      import Extractor
from xpedite.types              import DataSource

LOGGER = logging.getLogger(__name__)
APPINFO_FILE_NAME = 'appinfo.txt'

class Collector(Extractor):
  """Parses sample files to gather time and pmu counters"""

  def __init__(self, counterFilter):
    """
    Constructs an instance of collector

    :param counterFilter: a filter to exclude compromised or unused counters
    :type counterFilter: xpedite.filter.TrivialCounterFilter

    """
    Extractor.__init__(self, counterFilter)
    self.samplesFileWildcard = 'samples-[0-9]*.csv'
    self.samplesFilePattern = re.compile(r'samples-(\d+)\.csv')

  @staticmethod
  def formatPath(path, maxChars):
    """
    Trims path to at most max characters

    :param path: file system path
    :param maxChars: maximum allowed characters for the path

    """
    return path if len(path) < maxChars else '...' + path[-maxChars:]

  def listSampleFiles(self, path):
    """
    Lists sample files in sorted order

    :param path: path to directory containing sample files

    """
    def orderByName(fileName):
      """
      Sorts files by lexographical order of their names

      :param fileName: Name of the file

      """
      match = self.samplesFilePattern.findall(fileName)
      if match and len(match) >= 1:
        return int(match[0])
      else:
        raise RuntimeError('failed to extract sequence no from report file ' + fileName)

    fileNames = fnmatch.filter(os.listdir(path), self.samplesFileWildcard)
    fileNames = list(sorted(fileNames, key=orderByName))
    filePaths = (os.path.join(path, fileName) for fileName in fileNames)
    return filePaths

  @staticmethod
  def gatherDataSource(path):
    """
    Gathers appinfo and samples dir to build a data source

    :param path: path to directory with txn data

    """
    appInfoPath = os.path.join(path, APPINFO_FILE_NAME)
    if not os.path.isfile(appInfoPath):
      LOGGER.error('skipping data source %s - detected missing appinfo file %s', path, APPINFO_FILE_NAME)
      return None

    directories = next(os.walk(path))[1]
    if len(directories) > 1:
      LOGGER.error('skipping data source %s - detected more than one (%d) directories', path, len(directories))
      return None
    return DataSource(appInfoPath, os.path.join(path, directories[0]))

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
    self.loadSamples(loader, appInfo.probes, dataSource.samplePath)
    loader.endCollection()

  def loadSamples(self, loader, probes, path):
    """
    Loads counters for a profile session from csv sample files

    :param loader: Loader to build transactions out of the counters
    :param probes: A list of probes associated with samples in a file
    :param path: Path to sample file for a thread with counter data in csv format

    """
    recordCount = 0
    reportDirs = list(sorted(os.listdir(path)))
    for threadInfo in reportDirs:
      fields = threadInfo.split('-')
      if len(fields) < 2:
        raise Exception('Datasource {} missing tls storage info {}\n'.format(reportDirs, threadInfo))
      threadId = fields[0]
      dirPath = os.path.join(path, threadInfo)
      if os.path.isdir(dirPath):
        loader.beginLoad(threadId, fields[1])
        for filePath in self.listSampleFiles(dirPath):
          recordCount += self.loadCounters(threadId, loader, probes, filePath)
        loader.endLoad()
    self.logCounterFilterReport()
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
