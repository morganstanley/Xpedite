"""
Module to load appInfo data.

Xpedite frameworks logs details on target process in appInfo file.
The module loads the follwing details about a target xpedite process
  1. pid         - process id of the main thread of the target process
  2. port        - port number for xpedite profiler tcp socket
  3. binary path - path of the target binary
  4. probes      - List of  instrument xpedite probes

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import logging
from xpedite.util.probeFactory import ProbeFactory

LOGGER = logging.getLogger(__name__)

class AppInfo(object):
  """
  Stores appInfo details of a target application

  The AppInfo forms the link between Xpedite profiler and a running instance of an application.
  This class members store data used to uniquely identify and profile a target process.

  """

  INDEX_PID = 0
  INDEX_PORT = 1
  INDEX_EXECUTABLE_PATH = 2
  INDEX_TSC_HZ = 3
  MIN_RECORD_COUNT = 4

  def __init__(self, path, workspace=None):
    """Loads AppInfo from file in the given path"""
    self.path = path
    self.pid = None
    self.port = None
    self.executablePath = None
    self.executableName = None
    self.tscHz = None
    self.probes = None
    self.workspace = workspace

  def load(self):
    """
    Loads AppInfo object from file

    This method parses a text file encoded in the following format
    Each line holds a key value pair using colon (:) as a delimiter.
    1. Line#1 stores pid of the target process (pid: 36434)
    2. Line#2 stores port number of the tcp listener in the target process (port: 33041)
    3. Line#3 stores path of the target application's executable file (binary: install/bin/xpediteDemo)
    4. Rest of the file stores a record per line for each of the instrumented probes in the process

    """
    records = None
    with open(self.path) as fileHandle:
      records = fileHandle.read().splitlines()

    if records and len(records) >= self.MIN_RECORD_COUNT:
      pidInfo = records[self.INDEX_PID].split()
      if len(pidInfo) >= 2:
        self.pid = pidInfo[1]
      else:
        self.raiseError(
          'failed to load pid from appinfo from file {} | line {}'.format(self.path, records[self.INDEX_PID])
        )

      portInfo = records[self.INDEX_PORT].split()
      if len(portInfo) >= 2:
        self.port = portInfo[1]
      else:
        self.raiseError(
          'failed to load port from appinfo from file {} | line {}'.format(self.path, records[self.INDEX_PORT])
        )

      executablePath = records[self.INDEX_EXECUTABLE_PATH].split()
      if len(executablePath) >= 2:
        self.executablePath = executablePath[1]
        self.executableName = os.path.basename(self.executablePath)
      else:
        self.raiseError(
          'failed to load binary path from appinfo from file {} | line {}'.format(
           self.path, records[self.INDEX_EXECUTABLE_PATH]
          )
        )

      tscHzInfo = records[self.INDEX_TSC_HZ].split()
      if len(tscHzInfo) >= 2:
        self.tscHz = int(tscHzInfo[1])
      else:
        self.raiseError(
          'failed to load tscHz from appinfo from file {} | line {}'.format(self.path, records[self.INDEX_TSC_HZ])
        )

      self.probes = ProbeFactory(self.workspace).buildFromRecords(records[self.MIN_RECORD_COUNT:])
    else:
      self.raiseError('failed to load appinfo from file {} | missing mandatory records'.format(self.path))

    return self

  @staticmethod
  def raiseError(errmsg):
    """
    Logs the given error message and throws an exception

    :param errmsg: error message to log

    """
    LOGGER.error(errmsg)
    raise Exception(errmsg)
