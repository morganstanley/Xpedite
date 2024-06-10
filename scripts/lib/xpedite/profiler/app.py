"""
Handle to connect and interact with target application

This module implements the logic to setup and teardown a Xpedite profiling session.
It also implements an admin interface, to configure and enable probes in target application.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import re
import time
import logging
from xpedite.profiler.environment import Environment, RemoteEnvironment
from xpedite.transport.net        import isIpLocal
from xpedite.types.dataSource     import CsvDataSourceFactory

LOGGER = logging.getLogger(__name__)

class XpediteApp(object):
  """
  Handle to interact with profiling target

  This class stores attributes needed to attach and interact with a target application

  The connection is established and terminated on invocation of start() and stop()
  methods respectively. Once established, the connection is kept alive with heartbeats.

  The following commands are exchanged during a profile session.
  1. beginProfile - Begin sample collection in target application
  2. endProfile   - End sample collection in target application
  3. ping - Exchanges heartbeats to keep profiler's tcp connection  alive
  4. admin - Commands to enable, disable and query probe status

  """

  def __init__(self, name, ip, appInfoPath, dryRun=False, workspace=None):
    """
    Constructs an instance of Xpediate App

    :param name: Name of the target application
    :type name: str
    :param ip: Name or ip address of the host where target app is running
    :type ip: str
    :param appInfoPath: Path to the appInfo file of the target application
    :type appInfoPath: str
    :param dryRun: Flag to enable simulation of target application
    :type dryRun: bool
    """

    self.name = name
    self.ip = ip
    self._appInfoPath = appInfoPath
    self.dryRun = dryRun
    self.env = None
    self.runId = None
    self.sampleFilePath = None
    self.workspace = workspace
    self.dataSource = None

  def __getattr__(self, name):
    if self.env:
      return getattr(self.env, name)
    raise Exception('app not started')

  def sampleFilePattern(self):
    """Returns wildcard pattern for the xpedite sample files"""
    if self.sampleFilePath:
      return self.sampleFilePath
    return '/dev/shm/xpedite-*-{}-[0-9]*.data'.format(self.env.pid)

  def beginProfile(self, pollInterval, samplesFileSize, timeout=10):
    """
    Sends command to begin sample collection in the target application

    :param pollInterval: Heartbeat interval (seconds)
    :param timeout: Maximum time to await a response from app (Default value = 10 seconds)

    """
    self.runId = int(time.time())
    self.sampleFilePath = '/dev/shm/xpedite-{}-{}-*.data'.format(self.name, self.runId)
    rc = self.env.admin(
      'BeginProfile --samplesFilePattern {} --pollInterval {} --samplesDataCapacity {}'.format(
      self.sampleFilePath, pollInterval, samplesFileSize if samplesFileSize else 0), timeout)
    if rc:
      errmsg = 'failed to begin profiling - {}'.format(rc)
      raise Exception(errmsg)
    return True

  def endProfile(self, timeout=10):
    """
    Sends command to end sample collection in target application

    :param timeout: Maximum time to await a response from app (Default value = 10 seconds)

    """
    return len(self.env.admin('EndProfile', timeout)) == 0

  def ping(self, keepAlive=False, timeout=10):
    """
    Pings target app to keep connections alive

    :param keepAlive: Flag to enable heartbeating for rpyc connection (Default value = False)
    :param timeout:  Maximum time to await a response form app (Default value = 10 seconds)

    """
    if keepAlive:
      self.keepAlive()
    return self.env.admin('Ping', timeout) == 'hello'

  def start(self):
    """
    Loads appInfo and establishes tcp connection to the target application.

    This method builds an enviroment instance, to seamlessly establish a local or remote tcp connection
    with the target application

    """
    isLocal = isIpLocal(self.ip)
    LOGGER.debug('Starting xpedite client | %s ip - %s', 'LOCAL' if isLocal else 'REMOTE', self.ip)
    LOGGER.debug('app info file - %s', self._appInfoPath)
    self.env = (
      Environment(self.ip, self._appInfoPath, self.dryRun, self.workspace) if isLocal
      else RemoteEnvironment(self.ip, self._appInfoPath, self.dryRun, self.workspace)
    )
    self.env.__enter__() # pylint: disable=unnecessary-dunder-call

  def stop(self):
    """Disconnects and detaches from target application"""
    if self.env:
      self.env.__exit__(None, None, None)
    LOGGER.debug('Stopped xpedite client')

  def restart(self):
    """Disconnects and reconnects to target application"""
    self.stop()
    self.start()

  def __enter__(self):
    self.start()
    return self

  def __exit__(self, objType, value, traceback):
    self.stop()

def pingApp(app):
  """
  Pings target application with retry logic

  :param app: an instance of xpedite app, to interact with target application
  :type app: xpedite.profiler.app.XpediteApp

  """
  import socket
  errMsg = None
  try:
    if app.ping():
      return
  except socket.timeout:
    errMsg = 'timed out pinging application'
  except socket.herror as hError:
    errMsg = 'encountered a host error: {}'.format(str(hError))
  except socket.gaierror as gaiError:
    errMsg = 'encountered an address-related error: {}'.format(str(gaiError))
  except socket.error as socketError:
    errMsg = 'encounter a socket error: {}'.format(str(socketError))

  LOGGER.warning('restarting xpedite client - application is not responding to ping - %s', errMsg)
  app.restart()

  try:
    if app.ping():
      return
  except:
    msg = 'xpedite can no longer connect to application. Is your application running ?'
    LOGGER.exception(msg)
    raise Exception(msg)

class XpediteDormantApp(XpediteApp):
  """
  Dormant app used to regenerate reports from recorded profiles

  This class simulates interation with target application to
  recreate reports from samples collected in past profiling sesions

  """

  def __init__(self, name, ip, appInfoPath, runId=None, dataSourcePath=None, workspace=None):
    """Constructs an instance of XpediteDormantApp"""
    dataSource = CsvDataSourceFactory().gather(dataSourcePath) if dataSourcePath else None
    if dataSource:
      LOGGER.warning('Data source detected. overriding appinfo to %s', dataSource.appInfoPath)
      appInfoPath = dataSource.appInfoPath
    XpediteApp.__init__(self, name, ip, appInfoPath, dryRun=True, workspace=workspace)
    self.dataSource = dataSource
    self.runId = runId

  def beginProfile(self, pollInterval, samplesFileSize, timeout=10):
    """
    Override for simulation of begin profile

    :param pollInterval: Heartbeat interval (seconds)
    :param timeout: Maximum time to await a response from app (Default value = 10 seconds)

    """
    return True

  def endProfile(self, timeout=10):
    """
    Override for simulation of end profile

    :param timeout: Maximum time to await a response from app (Default value = 10 seconds)

    """
    return True

  def ping(self, keepAlive=False, timeout=10):
    """
    Override for simulation of heartbeats

    :param keepAlive: Flag to enable heartbeats for the rpyc connection (Default value = False)
    :param timeout: Maximum time to await a response from app (Default value = 10 seconds)

    """
    return True

  def sampleFilePattern(self):
    """
    Builds wildcard pattern to locate sample files for a given profiling session

    Encodes runid in wildcard pattern to match files for a given profiling session.

    """
    if self.sampleFilePath:
      return self.sampleFilePath
    return '/dev/shm/xpedite-*-{}-[0-9]*.data'.format(self.runId)

class TargetApp(object):
  """
  Target app provides logic to launch a profiling target

  The class lauches a target application and mointors the standard output
  to detect successful intialization of xpedite framework.

  """

  LOG_FILE_NAME = 'app.log'
  PATTERN = re.compile('.*Listener xpedite.*listening for incoming connections.*')

  def __init__(self, args):
    """
    Constructs an instance of target app

    :param args: path to binary and command line arguments

    """
    self.args = args
    self.proc = None
    self.logFileHandle = None

  def __enter__(self):
    """
    Starts a target application and awaits framework initialization

    The methods starts the target process with stdout redirected to a logfile.
    The log file is monitored for framework initialization messages

    """
    import subprocess
    self.logFileHandle = open(TargetApp.LOG_FILE_NAME, 'w')
    LOGGER.debug('starting target app %s', self.proc)
    self.proc = subprocess.Popen(self.args, stdout=self.logFileHandle)
    maxAttempts = 10
    size = 0
    while size <= 0 and maxAttempts > 0:
      size = os.stat(TargetApp.LOG_FILE_NAME).st_size
      LOGGER.debug(
        'attempt %d checking size of app info %s - %d bytes', 11 - maxAttempts, TargetApp.LOG_FILE_NAME, size
      )
      time.sleep(1)
      maxAttempts -= 1
    if size <= 0:
      errmsg = 'cannot locate app info file - {}'.format(TargetApp.LOG_FILE_NAME)
      LOGGER.error(errmsg)
      raise Exception(errmsg)
    with open(TargetApp.LOG_FILE_NAME, 'r') as fileHandle:
      LOGGER.debug('opened log %s', TargetApp.LOG_FILE_NAME)
      for line in fileHandle:
        LOGGER.debug('\t%s', line)
        if re.match(TargetApp.PATTERN, line):
          break
    return self

  def __exit__(self, objType, value, traceback):
    """Stops the target application"""
    LOGGER.debug('stopping target app')
    self.proc.kill()
    self.logFileHandle.close()
