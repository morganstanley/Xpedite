"""
Module to support asynchronous loading of profile data.

This module uses a thread pool, to recreate profile object from marshalled data.
Asynchronous loading is used to speedup time to load a new xpedite shell.

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
import time
from concurrent      import futures
import logging
from enum            import Enum
from xpedite.jupyter import PROFILES_KEY

LOGGER = logging.getLogger(__name__)

class ProfileStatus(Enum):
  """Enumeration of different profile load status"""
  LoadInProgress = 1
  LoadComplete = 2
  LoadFailed = 3

class Context(object):
  """Context to hold data for current profile session"""

  fileKey = 'xpdFileName'
  dataPathKey = 'XPEDITE_DATA_PATH'
  xpediteDataPath = os.getenv(dataPathKey)

  def __init__(self):
    from xpedite.analytics.conflator import Conflator
    self.conflator = Conflator()
    self._profiles = None
    self.profileState = None
    self.txn = None
    self.executor = futures.ThreadPoolExecutor(max_workers=1)
    self.dataFile = None
    self.isRealtime = False

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
    self._profiles = self.loadProfiles()
    self.txn = Txn(self._profiles.pmcNames) if self._profiles else None
    self.profileState = ProfileStatus.LoadComplete if self._profiles else ProfileStatus.LoadFailed

  def initialize(self, notebookName, cb=None, isRealtime=None):
    """
    Initialize context

    :param notebookName: name of the notebook
    :param cb:  load completion call back (Default value = None)

    """
    from xpedite.jupyter import buildXpdName
    self.dataFile = buildXpdName(os.path.join(Context.xpediteDataPath, notebookName))
    def doLoad():
      """Deleate to invoke the given callback, after context loading"""
      self.loadProfileAsync()
      if cb:
        cb(self)
    self.executor.submit(doLoad)
    self.isRealtime = isRealtime

  @property
  def profiles(self):
    """Returns profiles from context, awaiting async load"""
    if self.profileState == ProfileStatus.LoadFailed:
      LOGGER.error('profile data failed to load')
      raise Exception('Failure to get global profiles from future')
    elif self.profileState is None:
      LOGGER.error('Failure to set profile state')
      raise Exception('Failure to set profile state')
    elif self.profileState == ProfileStatus.LoadComplete:
      return self._profiles
    else:
      count = 0
      if not self.isRealtime:
        while self.profileState == ProfileStatus.LoadInProgress:
          time.sleep(.5)
          if self.profileState == ProfileStatus.LoadComplete:
            self.executor.shutdown()
            self.executor = None
            break
          count += 1
          if count >= 60:
            LOGGER.error('Loading profiles has timed out.')
            raise Exception('Timeout loading profiles.')
    return self._profiles

context = Context() # pylint: disable=invalid-name
