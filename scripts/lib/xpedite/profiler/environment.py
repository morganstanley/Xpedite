"""
Module to abstract host details (local vs remote) of the target application.

This module is used to hide the complexities in profiling applications running
in a remote host.
The module normalizes the interface used for
  1. locating and loading of  appInfo file
  2. collecting info about hardware and operating system
  3. enabling performance counters

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import logging
import tempfile
from xpedite.transport.remote import Remote
from xpedite.types            import CpuInfo
from xpedite.pmu              import pmuctrl
from xpedite.pmu.pmuctrl      import PMUCtrl
from xpedite.transport        import DatagramClient
from xpedite.profiler.appInfo import AppInfo
from xpedite.types            import InvariantViloation

LOGGER = logging.getLogger(__name__)

class ProxyEnvironment(object):
  """
  Collects and caches environment details for a profiling target.

  This class is used to collect and store environment details like
  cpu info, os version, kernel boot parameters etc ..

  """

  def __init__(self):
    """Constructs an instance of ProxyEnvironment"""
    self.fullCpuInfo = None
    self.cpuId = None
    self.osUname = None
    self.bootParam = None
    self.pmuCtrl = None
    self.keepAliveCount = 0

  @staticmethod
  def gatherFiles(pattern):
    """
    Method to build a list of files matching the given pattern

    :param pattern: Wild card pattern for files to be collected

    """
    import glob
    return list(glob.iglob(pattern))

  def getFullCpuInfo(self):
    """Builds and caches cpu info"""
    if not self.fullCpuInfo:
      from xpedite.util import getCpuInfo
      self.fullCpuInfo = getCpuInfo()
    return self.fullCpuInfo

  def getCpuId(self):
    """Discovers and caches cpu model"""
    if not self.cpuId:
      from xpedite.util import getCpuId
      self.cpuId = getCpuId(self.getFullCpuInfo())
    return self.cpuId

  def getOsUname(self):
    """Discovers and caches os build version"""
    if not self.osUname:
      self.osUname = os.uname()
    return self.osUname

  def getBootParam(self):
    """Reads and caches kernel boot parameters"""
    if not self.bootParam:
      with open('/proc/cmdline', 'r') as fileHandle:
        self.bootParam = fileHandle.read()
    return self.bootParam

  def isDriverLoaded(self): # pylint: disable=no-self-use
    """Returns status of xpedite device driver"""
    return pmuctrl.isDriverLoaded()

  def enablePMU(self, eventsDb, cpuSet, events):
    """
    Enables user space pmc collection for the given cpuSet

    :param eventsDb: A database of programmable performance counters
    :type eventsDb: xpedite.pmu.eventsDb.EventsDb
    :param cpuSet: A list of cpu cores to enable pmc collection
    :param events: A user supplied list of pmc events to be programmed

    """
    if not cpuSet or len(cpuSet) <= 0:
      raise Exception('Invalid argument - cpu set missing. need explicit cpu set to enable pmu')
    self.pmuCtrl = PMUCtrl(eventsDb)
    self.pmuCtrl.__enter__() # pylint: disable=unnecessary-dunder-call
    return self.pmuCtrl.enable(cpuSet, events)

  def disablePMU(self):
    """Disables user space pmc collection and restores cpu core to original state"""
    if self.pmuCtrl:
      self.pmuCtrl.__exit__() # pylint: disable=unnecessary-dunder-call

  @staticmethod
  def getVmStats(pid):
    """
    Collects virtual memory usage statistics for a target process

    :param pid: pid of the target process

    """
    result = {}
    try:
      lines = None
      with open('/proc/{}/status'.format(pid), 'r') as fileHandle:
        lines = [line.rstrip('\n') for line in fileHandle]
    except IOError:
      return result
    for line in lines:
      if not line.startswith('Vm'):
        continue
      parts = line.split()
      result[parts[0]] = parts[len(parts) - 2]
    return result

  def keepAlive(self):
    """Initiates a method call to keep the rpyc connection alive for long profiling sessions"""
    self.keepAliveCount += 1

class Environment(object):
  """Provides logic to interact with target process running in the same host"""

  def __init__(self, ip, appInfoPath, dryRun, workspace=None):
    self.ip = ip
    self.appInfo = AppInfo(appInfoPath, workspace)
    self.proxy = ProxyEnvironment()
    self.dryRun = dryRun
    self.client = None

  @property
  def appInfoPath(self):
    """Path to the app info file"""
    return self.appInfo.path

  @property
  def pid(self):
    """Pid of the target process"""
    return self.appInfo.pid

  @property
  def port(self):
    """Port number of the xpedite lister in the target process"""
    return self.appInfo.port

  @property
  def executablePath(self):
    """Path to the executable file of the target process"""
    return self.appInfo.executablePath

  @property
  def executableName(self):
    """Name of the executable file of the target process"""
    return self.appInfo.executableName

  @property
  def tscHz(self):
    """Returns estimated frequency of cpu time stamp counter"""
    return self.appInfo.tscHz

  @property
  def probes(self):
    """List of probes instrumented in the target process"""
    return self.appInfo.probes

  def gatherFiles(self, pattern):
    """
    Gathers files matching the given pattern

    :param pattern: Wild card pattern for files to be collected

    """
    return self.proxy.gatherFiles(pattern)

  def getCpuClockFrequencyRaw(self):
    """Returns the raw cpu clock frequency for localhost"""
    rawHz = self.getFullCpuInfo().advertisedHz
    if int(self.tscHz / 10**9) != int(rawHz / 10**9):
      LOGGER.error('Detected mismatch in estimated TSC (%s) vs raw frequency (%s)', self.tscHz, rawHz)
    return rawHz

  def getCpuInfo(self):
    """Returns cpu info for localhost"""
    return CpuInfo(self.getCpuId(), self.getCpuClockFrequencyRaw())

  def getFullCpuInfo(self):
    """Returns full cpu info of localhost"""
    return self.proxy.getFullCpuInfo()

  def getCpuId(self):
    """Returns cpu model of localhost"""
    return self.proxy.getCpuId()

  def getOsUname(self):
    """Returns kernel version of operating system running in target host"""
    return self.proxy.getOsUname()

  def getBootParam(self):
    """Returns kernel boot paramters of localhost"""
    return self.proxy.getBootParam()

  def isDriverLoaded(self):
    """Returns status of xpedite device driver"""
    return self.proxy.isDriverLoaded()

  def enablePMU(self, eventsDb, cpuSet, events):
    """
    Enables PMU counters for the given cpu set in target host

    :param eventsDb: A database of programmable performance counters
    :type eventsDb: xpedite.pmu.eventsDb.EventsDb
    :param cpuSet: A list of cpu cores to enable pmc collection
    :param events: A user supplied list of pmc events to be programmed

    """
    if not self.isDriverLoaded():
      (eventSet, request) = PMUCtrl.buildPerfEventsRequest(eventsDb, events)
      if eventSet and request:
        LOGGER.warning('xpedite device driver not loaded - falling back to perf events api')
        LOGGER.debug('sending request (%d bytes) to xpedite [%s]', len(request), request)
        rc = self.admin('ActivatePerfEvents --data {}'.format(request))
        if rc:
          raise Exception(rc)
        return eventSet
    return self.proxy.enablePMU(eventsDb, cpuSet, events)

  def disablePMU(self):
    """Disables user space pmc collection and restores cpu core to orginal state"""
    return self.proxy.disablePMU()

  def getVmStats(self):
    """
    Collects virtual memory usage statistics for a target process

    :param pid: pid for the target process

    """
    return self.proxy.getVmStats(self.pid)

  def keepAlive(self):
    """Initiates a method call to keep the rpyc connection alive for long profiling sessions"""
    return self.proxy.keepAlive()

  def admin(self, cmd, timeout=10):
    """
    Sends command to enable/disable/query probe status

    :param cmd: Command to execute in target application
    :param timeout: Maximum time to await a response from app (Default value = 10 seconds)

    """
    self.client.send(cmd)
    pdu = self.client.readFrame(timeout)
    if len(pdu) < 5 or pdu[4] != '|':
      raise InvariantViloation('Invalid response - pdu not in expected format \n{}\n'.format(pdu))
    status = pdu[3]
    if not str.isdigit(status):
      raise InvariantViloation('Invalid response - status code not in expected format \n{}\n'.format(pdu))
    result = pdu[5:] if len(pdu) > 5 else ''
    if int(status):
      raise Exception('Failed to execute request - {}'.format(result))
    return result

  def estimateTscHz(self, timeout=10):
    """Sends request to estimate frequency of cpu time stamp counter"""
    return self.admin('TscHz', timeout)

  def __enter__(self):
    """Instantiates a tcp client and connects to the target application"""
    if self.client:
      raise InvariantViloation('environment already in use')

    self.loadAppInfo()
    if not self.dryRun:
      self.client = DatagramClient(self.ip, self.port)
      self.client.connect()
    return self

  def loadAppInfo(self):
    """Loads application info of the target process"""
    self.appInfo.load()
    LOGGER.debug('resolved target pid to %s', self.pid)
    LOGGER.debug('resolved target port to %s', self.port)
    LOGGER.debug('resolved binary path to %s', self.executablePath)

  def __exit__(self, objType, value, traceback):
    """Disconnects tcp connection to the target app"""
    if not self.dryRun:
      if self.client:
        self.client.close()
        self.client = None

class RemoteEnvironment(Environment):
  """
  Provides logic to interact with target process running in a remote host

    This class uses rpyc connection to provide the following services
    1. Gathers environment info from a remote machine
    2. Copies appInfo and sample files from remote host to local host
  """

  def __init__(self, ip, appInfoPath, dryRun, workspace=None):
    """
    Constructs an instance of Remote environment

    :param ip: Name or ip address of the host where target application is running
    :type ip: str
    :param appInfoPath: Path to appInfo file of the target process
    :type appInfoPath: str
    :param dryRun: Flag to enable simlulation of target application
    :type dryRun: bool

    """
    Environment.__init__(self, ip, appInfoPath, dryRun, workspace)
    LOGGER.debug('Registered rpyc channel to %s ', ip)
    from xpedite.util import makeLogPath
    self.remote = Remote(ip, makeLogPath('remote'), chdir=False)

  def __enter__(self):
    """
    Instantiates a tcp client and connects to target application

    The methods executes the following actions
    1. Builds a rpyc connection to the target host
    2. Copies appInfo file from remote host to a temporary filesystem path in localhost
    3. Loads AppInfo object from the copied file
    4. Establishes a tcp connection to the target application

    """
    from xpedite.transport.remote import deliver
    self.remote.__enter__()
    self.proxy = deliver(self.remote.connection, self.proxy)
    result = self.gatherFiles(self.appInfo.path)
    if len(result) != 1:
      errmsg = 'Failed to gather app info file {} from remote host {} - got {}'.format(
        self.appInfo.path, self.ip, result
      )
      LOGGER.error(errmsg)
      raise Exception(errmsg)
    self.appInfo = AppInfo(result[0], self.appInfo.workspace)
    LOGGER.debug('Copied appinfo files from remote host to %s', self.appInfo.path)
    Environment.__enter__(self)
    LOGGER.debug('initializing remote environment - delivered proxy to %s ', self.ip)
    return self

  def __exit__(self, objType, value, traceback):
    """Disconnects tcp connection to the target app and terminates rpyc session"""
    Environment.__exit__(self, None, None, None)
    self.remote.__exit__(None, None, None)

  def gatherFiles(self, pattern):
    """
    Copies files matching pattern from remote host to a temp filesystem path in localhost

    :param pattern: Wild card pattern for files to be collected

    """
    remotePaths = self.proxy.gatherFiles(pattern)
    tmpRoot = tempfile.mkdtemp(prefix='xpedite', suffix='Remote', dir='/tmp')
    files = []
    for remotePath in remotePaths:
      localPath = tmpRoot + os.path.abspath(remotePath)
      if os.path.exists(localPath):
        if not os.path.isdir(localPath):
          os.remove(localPath)
      else:
        os.makedirs(localPath)
      filePath = os.path.join(localPath, os.path.basename(remotePath))
      import rpyc
      rpyc.utils.classic.download(self.remote.connection, remotePath, filePath)
      files.append(filePath)
    return files
