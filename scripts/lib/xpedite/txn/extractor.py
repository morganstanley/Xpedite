"""
Extractor to extract and collect xpedite samples data

This module is used to load counter data from xpedite binary sample files.
A decoder is used to open and extract timing and pmc data captured by
probes in the target application.
Each such decoded record is inturn used to construct a Counter object for
transaction building.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import re
import time
import logging
import subprocess
import fcntl
from xpedite.types      import Counter, DataSource
from xpedite.util       import makeLogPath, mkdir
from threading import Thread

LOGGER = logging.getLogger(__name__)

class Extractor(object):
  """Parses sample files to load counters for the current profile session"""

  moduleDirPath = os.path.dirname(os.path.abspath(__file__))
  samplesLoader = '{}/../../../bin/xpediteSamplesLoader'.format(moduleDirPath)

  def __init__(self, counterFilter):
    """
    Constructs a new instance of extractor

    :param counterFilter: Filter to exclude out compromised or unused counters
    :type counterFilter: xpedite.filter.TrivialCounterFilter

    """
    self.binaryReportFilePattern = re.compile(r'[^\d]*(\d+)-(\d+)-([0-9a-fA-F]+)\.data')
    self.counterFilter = counterFilter
    self.orphanedRecords = []
    self.stopProfiling = False

  def stopExtracting(self, button): #pylint: disable=unused-argument
    """
    Receive and respond signals from the stop button on jupyter interface.
    """
    LOGGER.completed('Live Profiling Stopped')
    self.stopProfiling = True

  def gatherCountersLive(self, app, loader, stopButton):
    """
    Gathers time and pmu counters from sample files for the current profile session in realtime mode

    :param app: Handle to the instance of the xpedite app
    :type app: xpedite.profiler.environment.XpediteApp
    :param loader: Loader to build transactions out of the counters

    """
    pattern = app.sampleFilePattern()
    patternList = pattern.split('/')[:-1]
    dataDir = '/'.join(patternList) + '/'
    LOGGER.info('scanning for samples files matching - %s', pattern)

    samplePath = makeLogPath('{}/{}'.format(app.name, app.runId))
    dataSource = DataSource(app.appInfoPath, samplePath)
    loader.beginCollection(dataSource)

    args = [self.samplesLoader]
    args.extend([str(app.runId), "REALTIME", dataDir])

    iterBegin = begin = time.time()
    extractor = subprocess.Popen(
      args,
      bufsize=2*1024*1024,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      universal_newlines=True
    )

    recordCount = 0
    timeLast = time.time()

    stopButton.on_click(self.stopExtracting)

    while True:

      if self.stopProfiling:
        extractor.kill()
        yield 1

      if time.time() - timeLast >= 0.02:
        timeLast = time.time()
        yield

      entry = readFromLoader(extractor, 'REALTIME')
      if entry is None:
        continue
      threadId, _, record = entry

      if recordCount > 0:
        self.loadCounter(threadId, loader, app.probes, record)
        elapsed = time.time() - iterBegin
        if elapsed >= 5:
          LOGGER.completed('\n\tprocessed %d counters | ', recordCount-1)
          iterBegin = time.time()

      recordCount += 1
      self.logCounterFilterReport()

      elapsed = time.time() - begin
      if self.orphanedRecords:
        LOGGER.warn('detected mismatch in binary vs app info - %d counters ignored', len(self.orphanedRecords))


  def gatherCounters(self, app, loader):
    """
    Gathers time and pmu counters from sample files for the current profile session in batch mode

    :param app: Handle to the instance of the xpedite app
    :type app: xpedite.profiler.environment.XpediteApp
    :param loader: Loader to build transactions out of the counters

    """
    pattern = app.sampleFilePattern()
    patternList = pattern.split('/')[:-1]
    dataDir = '/'.join(patternList) + '/'
    LOGGER.info('scanning for samples files matching - %s', pattern)

    samplePath = makeLogPath('{}/{}'.format(app.name, app.runId))
    dataSource = DataSource(app.appInfoPath, samplePath)
    loader.beginCollection(dataSource)

    args = [self.samplesLoader]
    args.extend([str(app.runId), "BATCH", dataDir])

    iterBegin = begin = time.time()
    extractor = subprocess.Popen(
      args,
      bufsize=2*1024*1024,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      universal_newlines=True
    )

    recordCount = 0

    while True:
      entry = readFromLoader(extractor, 'BATCH')
      if entry is None:
        break
      threadId, _, record = entry

      if recordCount > 0:
        self.loadCounter(threadId, loader, app.probes, record)

        elapsed = time.time() - iterBegin
        if elapsed >= 5:
          LOGGER.completed('\n\tprocessed %d counters | ', recordCount-1)
          iterBegin = time.time()

      recordCount += 1
      self.logCounterFilterReport()

      elapsed = time.time() - begin
      if self.orphanedRecords:
        LOGGER.warn('detected mismatch in binary vs app info - %d counters ignored', len(self.orphanedRecords))

      if loader.isCompromised() or loader.getTxnCount() <= 0:
        LOGGER.warn(loader.report())
      elif loader.isNotAccounted():
        LOGGER.debug(loader.report())

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
    fields = record
    if len(fields) < self.MIN_FIELD_COUNT:
      raise Exception('detected record with < {} fields - \nrecord: "{}"\n'.format(self.MIN_FIELD_COUNT, record))
    addr = fields[self.INDEX_ADDR]
    if addr not in probes:
      self.orphanedRecords.append(record)
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

def readFromLoader(extractor, mode):
  """
  read output from the C++ loader
  parse the record if a valid record is collected
  """
  if mode == 'REALTIME':
    fd = extractor.stdout.fileno()
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
    while True:
      try:
        fullRecord = extractor.stdout.readline()
      except: #pylint: disable=bare-except
        return
      break
  else:
    fullRecord = extractor.stdout.readline()

  if fullRecord.strip() == '':
    if extractor.poll() is not None:
      errmsg = extractor.stderr.read()
      if errmsg:
        raise Exception('failed to execute c++ {}'.format(errmsg))
    return

  recordList = fullRecord.split(',')
  if len(recordList) < 2:
    recordList.extend('')
    if extractor.poll() is not None:
      errmsg = extractor.stderr.read()
      if errmsg:
        raise Exception('failed to load {} - {}'.format(recordList, errmsg))
    return

  currentThreadId = recordList[0]
  currentTlsAddr = recordList[1]
  record = recordList[2:]

  return currentThreadId, currentTlsAddr, record
