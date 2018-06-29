"""
Package to self profile xpedite profiler

This package captures timing statistics for xpedite counter
collection and analytics logic.

The captured metrics are converted to call tree format for
visualization using kcachegrind.

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import sys
import pstats
import cProfile
import logging
import StringIO

LOGGER = logging.getLogger(__name__)

class CProfile(object):
  """Profiler to collect timing stats for xpedite report generation logic"""

  def __init__(self, path):
    #append .xpp to indicate which files are xpedite profile files
    self.path = path + '.xpp'
    self.cprofile = cProfile.Profile()

  def __enter__(self):
    self.enable()

  def __exit__(self, excType, excVal, excTb):
    self.disable()

  def enable(self):
    """Enables profiling"""
    self.cprofile.enable()
    LOGGER.info('Profiler enabled, writing .prof to file %s...\n', self.path)

  def disable(self):
    """Disaables profiling"""
    self.cprofile.disable()
    strIO = StringIO.StringIO()
    sortby = 'cumulative'
    ps = pstats.Stats(self.cprofile, stream=strIO).sort_stats(sortby)
    LOGGER.info('Profiler disabled, data written to %s', self.path)
    from xpedite.dependencies import Package, DEPENDENCY_LOADER
    if DEPENDENCY_LOADER.load(Package.PyProf2Calltree): # pylint: disable=no-member
      from pyprof2calltree  import convert # pylint: disable=import-error
      convert(ps, self.path)
      LOGGER.info('Open the report in KCachegrind to see the profile report')
