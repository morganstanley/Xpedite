"""
Class definitions used in gathering and loading binary and csv sample files

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import re
import fnmatch
import logging
from xpedite.dependencies import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Enum, Package.Six)
from enum import Enum # pylint: disable=wrong-import-position

APPINFO_FILE_NAME = 'appinfo.txt'
LOGGER = logging.getLogger(__name__)

class SampleFileFormat(Enum):
  """Format of the sample file"""

  BINARY = 1
  CSV = 2

  def __eq__(self, other):
    if other:
      return self.__dict__ == other.__dict__
    return None

class SampleFile(object):
  """Sample File for a thread"""

  def __init__(self, threadId, tlsAddr, path, fmt):
    self.threadId = threadId
    self.tlsAddr = tlsAddr
    self.path = path
    self.fmt = fmt

  def __repr__(self):
    return '{} sample file for thread id - {} | tlsAddr - {} | path - {}'.format(self.fmt, self.threadId,
        self.tlsAddr, self.path)

class DataSource(object):
  """A collection of sample files in a profile"""

  def __init__(self, appInfoPath, files):
    self.appInfoPath = appInfoPath
    self.files = files

  def __repr__(self):
    return 'Data Source - app info path - {} | files - {}'.format(self.appInfoPath, self.files)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class CsvDataSourceFactory(object):
  """Factory to create csv data source"""

  CSV_FILE_WILDCARD = 'samples-[0-9]*.csv'
  CSV_FILE_PATTERN = re.compile(r'samples-(\d+)\.csv')

  def _gatherSampleFiles(self, path):
    """
    Gather csv sample files in sorted order

    :param path: path to directory containing sample files

    """
    def orderByName(fileName):
      """
      Sorts files by lexographical order of their names

      :param fileName: Name of the file

      """
      match = self.CSV_FILE_PATTERN.findall(fileName)
      if match and len(match) >= 1:
        return int(match[0])
      raise RuntimeError('failed to extract sequence no from report file ' + fileName)

    files = []
    sampleDirs = list(sorted(os.listdir(path)))
    for threadInfo in sampleDirs:
      fields = threadInfo.split('-')
      if len(fields) < 2:
        raise Exception('Datasource {} missing tls storage info {}\n'.format(sampleDirs, threadInfo))
      dirPath = os.path.join(path, threadInfo)
      if os.path.isdir(dirPath):
        fileNames = fnmatch.filter(os.listdir(dirPath), self.CSV_FILE_WILDCARD)
        fileNames = list(sorted(fileNames, key=orderByName))
        filePaths = (os.path.join(dirPath, fileName) for fileName in fileNames)
        for filePath in filePaths:
          files.append(SampleFile(fields[0], fields[1], filePath, SampleFileFormat.CSV))
    return files

  def gather(self, path):
    """
    Gathers appinfo and sample files to build a data source

    :param path: path to directory with sample data

    """
    appInfoPath = os.path.join(path, APPINFO_FILE_NAME)
    if not os.path.isfile(appInfoPath):
      LOGGER.error('skipping data source %s - detected missing appinfo file %s', path, APPINFO_FILE_NAME)
      return None

    directories = next(os.walk(path))[1]
    if len(directories) > 1:
      LOGGER.error('skipping data source %s - detected more than one (%d) directories', path, len(directories))
      return None
    files = self._gatherSampleFiles(os.path.join(path, directories[0]))
    return DataSource(appInfoPath, files)

class BinaryDataSourceFactory(object):
  """Factory to create binary data source"""

  BINARY_FILE_PATTERN = re.compile(r'[^\d]*(\d+)-(\d+)-([0-9a-fA-F]+)\.data')

  def extractThreadInfo(self, samplesFile):
    """
    Extracts thread id/thread local storage address from name of the samples file

    :param samplesFile: Name of the samples file

    """
    match = self.BINARY_FILE_PATTERN.findall(samplesFile)
    if match and len(match[0]) > 2:
      return (match[0][1], match[0][2])
    return (None, None)

  def gather(self, app):
    """
    Gathers appinfo and binary sample files to build a data source

    :param path: path to directory with binary sample data

    """
    pattern = app.sampleFilePattern()
    LOGGER.info('scanning for samples files matching - %s', pattern)
    filePaths = app.gatherFiles(pattern)

    files = []
    for filePath in filePaths:
      (threadId, tlsAddr) = self.extractThreadInfo(filePath)
      if not threadId or not tlsAddr:
        raise Exception('failed to extract thread info for file {}'.format(filePath))
      files.append(SampleFile(threadId, tlsAddr, filePath, SampleFileFormat.BINARY))
    return DataSource(app.appInfoPath, files)
