"""
Adapter to collect profile data and html reports for jupyter integration

This module provides classes to collect profile, flot and html data for rendering in jupyter
The html reports are compressed and aggregated with adjacent flot data for reporting.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.util import compressText

class ZippedMarkup(object):
  """Class to store compressed html reports"""

  def __init__(self, markup):
    self.name = markup.name
    self.description = markup.description
    self.zContent = compressText(markup.content)

class Reportcell(object):
  """Class to store profile results for a category"""

  def __init__(self, flot):
    self.flot = flot
    self.htmlReport = []

class Result(object):
  """Class to store profile results for the current session"""

  def __init__(self, report):
    self.runId = report.runId
    self.profiles = report.profiles
    self.envReport = ZippedMarkup(report.envReport)
    self.reportName = report.profiles.name
    self.reportCells = []

    for category in report.categories.values():
      reportcell = Reportcell(category.histogram)
      for routeMarkup  in category.routes:
        reportcell.htmlReport.append(ZippedMarkup(routeMarkup))
      self.reportCells.append(reportcell)
