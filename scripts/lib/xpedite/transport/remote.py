"""
Remote running utility

Author: Dhruv Shekhawat, Morgan Stanley
"""
import os
import re
import sys
import time
import rpyc
import logging
import subprocess

LOGGER = logging.getLogger(__name__)

def deliver(remote, obj):
  """Delivers the given object to a remote host"""
  moduleDirPath = os.path.dirname(os.path.abspath(__file__))
  xpeditePath = os.path.join(moduleDirPath, '../..') # two level up
  if remote.modules.os.path.exists(xpeditePath):
    remote.modules.sys.path.append(xpeditePath)
  else:
    hostname = remote.modules.socket.gethostname()
    errmsg = 'xpedite suite {} is not installed in the remote host {}.'.format(xpeditePath, hostname)
    LOGGER.error(errmsg)
    raise Exception(errmsg)
  return rpyc.utils.classic.deliver(remote, obj)

def matchRegexpsInFile(logpath, logExtracts):
  """
  :returns: as well as any named groups they might have matched.

  """
  extractedValues = {}
  if not os.path.exists(logpath):
    return False, extractedValues

  extractsStatus = [False for regexp in logExtracts]

  with open(logpath, 'r') as log:
    for line in log:
      for pos, regexp in enumerate(logExtracts):
        match = regexp.match(line)
        if match:
          extractedValues.update(match.groupdict())
          extractsStatus[pos] = True
  return all(extractsStatus), extractedValues

class StdFiles(object):
  """stderr and stdout file creation and management"""

  def __init__(self, directory):
    """Create files and initialize fds"""
    self.errPath = os.path.join(directory, 'stderr')
    self.outPath = os.path.join(directory, 'stdout')

    #pylint: disable=consider-using-with
    self.err = open(self.errPath, 'w')
    self.out = open(self.outPath, 'w')

  def close(self):
    """Close fds"""
    self.err.close()
    self.out.close()

class Remote(object):
  """Remote running wrapper"""

  def __init__(self, host, directory, timeout=30, chdir=True, keepalive=False):
    """
    Create a remote rpyc server and connection

    :param host: target host
    :type host: str
    :param directory: log directory
    :type directory: str
    :param timeout: timeout in seconds
    :type timeout: int
    :param chdir: indicate if should change dir of remote connection to current directory
    :type chdir: bool
    :param keepalive: argument of rpyc.classic.connect to keep a long running connection alive.
    :type keepalive: bool
    """
    self.host = host
    self.proc = None
    self.port = None
    if not os.path.isdir(os.path.join(directory, host)):
      os.makedirs(os.path.join(directory, host))
    self.std = StdFiles(os.path.join(directory, host))
    self.connection = None
    self.pid = None
    self.timeout = timeout
    self.chdir = chdir
    self.keepalive = keepalive

  def __enter__(self):
    """
    Bring up the server and connect to it

    :return: the Remote itself
    :rtype: Remote
    """
    from xpedite.dependencies import binPath
    python = binPath('python')
    rpyproc = os.path.join(os.path.dirname(__file__), 'rpyc')

    if self.host == 'localhost':
      self.proc = subprocess.Popen([python, rpyproc], stderr=self.std.err, stdout=self.std.out)
    else:
      pluginPath = os.environ.get('XPEDITE_PLUGIN_PATH', '')
      env = 'export XPEDITE_PLUGIN_PATH={};'.format(pluginPath) if pluginPath else ''
      ssh = binPath('ssh')
      self.proc = subprocess.Popen([ssh, '-T', '-o', 'StrictHostKeyChecking=no', self.host, env, python, rpyproc],
                                   stderr=self.std.err,
                                   stdout=self.std.out)

    seconds, done, extracts = 0, False, {}
    while seconds < self.timeout and not done:
      done, extracts = matchRegexpsInFile(
                         self.std.errPath,
                         [re.compile('.*server started on .*:(?P<port>.*)'),
                          re.compile(r'.*pid\<(?P<pid>[0-9]+)\>')])
      time.sleep(0.2)
      seconds += 0.2

    if not done:
      self.kill()
      with open(self.std.errPath, 'r') as err:
        errStr = err.read()
      raise Exception('Timed out trying to launch rpyc on {0} - stderr contents: {1}'.format(
                        self.host, errStr))

    self.port = int(extracts['port'])
    self.pid = int(extracts['pid'])

    pwd = os.environ['PWD']
    self.connection = rpyc.classic.connect(self.host, self.port, keepalive=self.keepalive)
    self.connection.modules.sys.stdout = sys.stdout
    if self.chdir:
      self.connection.modules.os.chdir(pwd)
      self.connection.modules.os.environ['PWD'] = pwd
    return self

  def kill(self):
    """
    Terminate ssh connection and reap rpyc

    :returns: None
    :rtype: NoneType

    """
    self.proc.kill()
    self.proc.wait()
    if self.host != 'localhost':
      from xpedite.dependencies import binPath
      ssh = binPath('ssh')
      #pylint: disable=consider-using-with
      subprocess.Popen([ssh, '-T', '-o', 'StrictHostKeyChecking=no', self.host, 'kill', str(self.pid)]).wait()
    self.std.close()

  def __exit__(self, objType, value, traceback):
    """
    Bring down the server

    :return: None
    :rtype: NoneType
    """
    self.connection.close()
    self.connection = None
    self.kill()

  def __repr__(self):
    return self.host
