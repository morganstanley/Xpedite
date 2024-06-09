"""
Utility methods

This module provides utility methods for
  1. file system operations - make or clean up directories, touch files etc
  2. formating and normalizing strings
  3. collecting cpu and memory info
  4. etc ...

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from __future__           import division
import sys
import os
import time
import shutil
import tempfile
import logging
from collections          import OrderedDict
from xpedite.dependencies import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Six)

LOGGER = logging.getLogger(__name__)

def attachPdb(_, frame):
  """Attaches PDB instance to python process"""
  import pdb
  pdb.Pdb().set_trace(frame)

def timeAction(action, delegate):
  """
  Measures elapsed time for execution of a delegate

  :param action: Description of the delegate
  :param delegate: Callable to perform a task

  """
  begin = time.time()
  retVal = delegate()
  elapsed = time.time() - begin
  if elapsed > 10:
    LOGGER.warning('timed action exceeded threshold %s completed in %s.1f seconds', action, elapsed)
  return retVal

def shell(cmd, cwd=None, closeFds=True):
  """
  Executes a shell command

  :param cmd: Command string
  :param cwd: Current working directory (Default value = None)
  :param closeFds: If True, all file descriptors except 0, 1 and 2 will
    be closed before the child process is executed. Defaults to True.
  :returns: a (return code, std out, std error) triplet
  :rtype: tuple of int, str, str

  """
  import subprocess
  if cwd:
    #pylint: disable=consider-using-with
    process = subprocess.Popen(
      cmd, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE, cwd=cwd, close_fds=closeFds
    )
  else:
    process = subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=closeFds)
  stdout, stderr = process.communicate()
  return (process.returncode, stdout, stderr)

def makeUniqueId():
  """Returns an unique identifier for css selector"""
  return str(time.time()).replace('.', '_')

def mkTempFilePath():
  """Creates a temporary directory in the file system"""
  fd, tempPath = tempfile.mkstemp()
  os.close(fd)
  return tempPath

def formatHumanReadable(byteCount, suffix='B'):
  """Formats size using human friendly units"""
  for unit in ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi']:
    if abs(byteCount) < 1024.0:
      return '%3.1f %s%s' % (byteCount, unit, suffix)
    byteCount /= 1024.0
  return '%.1f %s%s' % (byteCount, 'Yi', suffix)

def persist(filePath, iterable, lineDelimiter=None):
  """
  Persists the given iterable to file system in text format

  :param filePath: path to the file
  :param iterable: Iterable to persist
  :param lineDelimiter: delimiter for lines in the file (Default value = None)

  """
  with open(filePath, 'w') as fileHandle:
    for item in iterable:
      fileHandle.write(str(item))
      if lineDelimiter:
        fileHandle.write(lineDelimiter)

def parseAddress(ipStr):
  """
  Parses ip address and port from a string

  :param ipStr: String with ip address and port delimited by colon

  """
  words = ipStr.split(':')
  if len(words) != 2:
    errMsg = 'ill-formatted address {}. expect address in format - <ip4-address>:<port>'.format(ipStr)
    LOGGER.error(errMsg)
    raise Exception(errMsg)
  return (words[0], int(words[1]))

def parsePort(port):
  """
  Converts port in string format to an int

  :param port: a string or integer value
  :returns: an integer port number
  :rtype: int

  """
  result = None
  try:
    result = int(port)
  except ValueError:
    import socket
    result = socket.getservbyname(port)
  return result

def promptUser():
  """Awaits a key press from console"""
  LOGGER.info('press return key to continue...')
  sys.stdin.read(1)

def removeFiles(path):
  """
  Removes a file or directory from file system

  :param path: path to file or directory

  """
  if os.path.isdir(path):
    shutil.rmtree(path, ignore_errors=True)
  else:
    try:
      os.remove(path)
    except OSError:
      pass

def mkdir(path, clean=False):
  """
  Creates a directory, optionally cleaning its contents

  :param path: Path to the directory
  :param clean: Flag to indicate clean up (Default value = False)

  """
  if clean:
    removeFiles(path)
  if os.path.exists(path):
    if os.path.isdir(path):
      if not os.access(path, os.W_OK):
        raise Exception('Path {} is not writable'.format(path))
    else:
      raise Exception('Path {} is not a directory'.format(path))
  else:
    os.makedirs(path)

def logPath(name=None):
  """
  Returns the path of xpedite log directory

  :param name: Optional suffix for the log path (Default value = None)

  """
  from xpedite.dependencies import CONFIG
  logpath = CONFIG.logDir
  if name:
    logpath = os.path.join(logpath, name)
  return logpath

def makeLogPath(name=None):
  """
  Creates the directory for storing log files

  :param name: Suffix for the log directory

  """
  path = logPath(name)
  mkdir(path)
  if not os.path.isdir(path):
    raise Exception('Could not create run directory {0}'.format(path))
  return path

def touch(path):
  """
  Touches a file in the given path

  :param path: path to touch

  """
  with open(path, 'a'):
    pass

def getCpuInfo():
  """Loads cpu info for localhost"""
  from xpedite.util.cpuInfo import CpuInfo
  return CpuInfo()

def getCpuId(cpuInfo=None):
  """Returns cpu model for localhost"""
  cpuInfo = cpuInfo if cpuInfo else getCpuInfo()
  return cpuInfo.cpuId

def meminfo(remoteConnection=None):
  """
  Loads memory info for localhost

  :param remoteConnection: handle to remote rpyc connection (Default value = None)

  """
  meminfoPath = '/proc/meminfo'
  if remoteConnection:
    tempFilePath = mkTempFilePath()
    import rpyc
    rpyc.utils.classic.download(remoteConnection, meminfoPath, tempFilePath)
    meminfoPath = tempFilePath
  meminfoMap = OrderedDict()
  with open(meminfoPath) as fileHandle:
    for line in fileHandle:
      meminfoMap[line.split(':')[0]] = line.split(':')[1].strip()
  return meminfoMap

def compressText(data):
  """
  returns compressed data in base64 format

  :param data: Data to be compressed

  """
  import zlib
  import base64
  import six
  compressor = zlib.compressobj(9, zlib.DEFLATED, zlib.MAX_WBITS | 16)
  zContent = compressor.compress(six.ensure_binary(data)) + compressor.flush()
  return base64.b64encode(zContent)

def loadTextFile(path):
  """
  Loads contents of the given file

  :param path: Path of the file to load

  """
  import six
  with open(path, 'rb') as fileHandle:
    return six.ensure_str(fileHandle.read())
