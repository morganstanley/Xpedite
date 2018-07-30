"""
Module containing applications for testing. This module has a AppLancher to set up a
remote environment for xpedite record, and builds a target app to create a process
in a local or remote host

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os

class TargetLauncher(object):
  """
  If a target application is running remotely, configure the remote environment on the remote host
  Create and launch the target application and xpedite application
  """

  def __init__(self, binary, profileInfo, txnCount, threadCount, workspace, remote=None):
    """
    Create and enter a temp directory which will be used to stored xpedite application information
    If a target application is being run remotely, TargetLauncher will create the temp directory on the remote host
    """
    import tempfile
    from xpedite.profiler.app import XpediteApp
    if remote:
      self.tempDir = remote.connection.modules.tempfile.mkdtemp()
      remote.connection.modules.os.chdir(self.tempDir)
    else:
      self.tempDir = tempfile.mkdtemp()
      os.chdir(self.tempDir)
    args = ([binary, '-c', '0', '-m', str(threadCount), '-t', str(txnCount)])
    self.targetApp = buildTargetApp(args, remote)
    appInfo = os.path.join(self.tempDir, 'xpedite-appinfo.txt')
    self.xpediteApp = XpediteApp(profileInfo.appName, profileInfo.appHost, appInfo, workspace=workspace)

  def __enter__(self):
    self.targetApp.__enter__()
    self.xpediteApp.start()
    return self

  def __exit__(self, objType, value, traceback):
    self.xpediteApp.stop()
    self.targetApp.__exit__(None, None, None)

def buildTargetApp(args, remote=None):
  """
  Deliver a target application to the remote host if running remotely
  """
  from xpedite.profiler.app       import TargetApp
  from xpedite.transport.remote   import deliver
  targetApp = TargetApp(args)
  if remote:
    targetApp = deliver(remote.connection, targetApp)
  return targetApp
