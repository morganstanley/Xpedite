"""
Module containing applications for testing. This module has a AppLancher to set up a
local or remote environment for Xpedite profiling, and builds a target app to create a process
in a local or remote host

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

class TargetLauncher(object):
  """
  If a target application is running remotely, configure the remote environment on the remote host
  Create and launch the target application and xpedite application
  """

  def __init__(self, context, scenario):
    """
    Create and enter a temp directory which will be used to stored xpedite application information
    If a target application is being run remotely, TargetLauncher will create the temp directory on the remote host
    """
    self.targetApp = scenario.makeTargetApp(context)
    self.xpediteApp = scenario.makeXpediteApp(context.workspace)

  def __enter__(self):
    self.targetApp.__enter__()
    self.xpediteApp.start()
    return self

  def __exit__(self, objType, value, traceback):
    self.xpediteApp.stop()
    self.targetApp.__exit__(None, None, None)
