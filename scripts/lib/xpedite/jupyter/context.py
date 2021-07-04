"""
Module to support asynchronous loading of profile data.

This module uses a thread pool, to recreate profile object from marshalled data.
Asynchronous loading is used to speedup time to load a new xpedite shell.

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
import traceback
import time
from concurrent      import futures
import logging
from enum            import Enum
from xpedite.jupyter import PROFILES_KEY, NOTEBOOK_EXT, DATA_FILE_EXT, DATA_DIR

LOGGER = logging.getLogger(__name__)

class ProfileStatus(Enum):
  """Enumeration of different profile load status"""
  LoadInProgress = 1
  LoadComplete = 2
  LoadFailed = 3

class Context(object):
  """Context to hold data for current profile session"""

  notebookPathKey = 'xpediteNotebookPath'
  xpediteHomeKey = 'XPEDITE_HOME_PATH'
  xpediteHome = os.getenv(xpediteHomeKey)

  def __init__(self):
    from xpedite.analytics.conflator import Conflator
    self.conflator = Conflator()
    self._profiles = None
    self.profileState = None
    self.txn = None
    #pylint: disable=consider-using-with
    self.executor = futures.ThreadPoolExecutor(max_workers=1)
    self.dataFile = None
    self.errMsg = None

  def loadProfiles(self):
    """Load profile data from Xpedite data file"""
    from xpedite.jupyter.xpediteData import XpediteDataReader
    with XpediteDataReader(self.dataFile) as xpd:
      profiles = xpd.getData(PROFILES_KEY)
    return profiles

  def loadProfileAsync(self):
    """Load profile data in a background thread"""
    from xpedite.jupyter.autoComplete import Txn
    self.profileState = ProfileStatus.LoadInProgress
    try:
      self._profiles = self.loadProfiles()
      self.txn = Txn(self._profiles.pmcNames) if self._profiles else None
    except Exception:
      self.errMsg = traceback.format_exc()
    finally:
      self.profileState = ProfileStatus.LoadComplete if self._profiles else ProfileStatus.LoadFailed

  def initialize(self, notebookPath, cb=None):
    """
    Initialize context

    :param notebookPath: relative path to the notebook
    :param cb:  load completion call back (Default value = None)

    """
    self.dataFile = self.buildXpdPath(notebookPath)
    def doLoad():
      """Deleate to invoke the given callback, after context loading"""
      self.loadProfileAsync()
      if cb:
        cb(self)
    self.executor.submit(doLoad)

  @staticmethod
  def buildXpdPath(notebookPath):
    """
    Builds path of xpedite data file, given a path to notebook file

    :param notebookPath: relative path of the jupyter notebook

    """
    (path, notebookName) = os.path.split(notebookPath)
    dataFileName = notebookName.replace(NOTEBOOK_EXT, DATA_FILE_EXT)
    return os.path.join(Context.xpediteHome, path, DATA_DIR, dataFileName)

  @property
  def profiles(self):
    """Returns profiles from context, awaiting async load"""
    if self.profileState == ProfileStatus.LoadFailed:
      errMsg = 'Failed to load transactions - {}'.format(self.errMsg)
      LOGGER.error(errMsg)
      raise Exception(errMsg)
    if self.profileState is None:
      errMsg = 'Invariant voilation - profile loading not yet inialized'
      LOGGER.error(errMsg)
      raise Exception(errMsg)
    if self.profileState == ProfileStatus.LoadComplete:
      return self._profiles
    count = 0
    while self.profileState == ProfileStatus.LoadInProgress:
      time.sleep(.5)
      if self.profileState == ProfileStatus.LoadComplete:
        if self.executor:
          self.executor.shutdown()
          self.executor = None
        break
      count += 1
      if count >= 60:
        LOGGER.error('Timeout loading transactions')
        raise Exception('Timeout loading transactions')
    return self._profiles

context = Context() # pylint: disable=invalid-name
