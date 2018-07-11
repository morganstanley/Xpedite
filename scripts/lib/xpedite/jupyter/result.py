"""
Adapter to collect profile data and html reports for jupyter integration

This module provides classes to collect profile, flot and html data for rendering in jupyter
The html reports are compressed and aggregated with adjacent flot data for reporting.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import zlib
import base64

class Report(object):
  """Class to store compressed html reports"""

  def __init__(self, name, description, zContent):
    self.name = name
    self.description = description
    self.zContent = zContent

class Reportcell(object):
  """Class to store profile results for a category"""

  def __init__(self, flot):
    self.flot = flot
    self.htmlReport = []

class Result(object):
  """Class to store profile results for the current session"""

  def __init__(self):
    self.reportCells = []
    self.envReport = None

  @staticmethod
  def _buildReport(title, description, content):
    zContent = zlib.compress(content)
    zContent = base64.b64encode(zContent)
    return Report(title, description, zContent)

  def attachEnvReport(self, title, description, content):
    """
    Attaches a html report with details about the test environment

    :param description: description of the report
    :param title: title of the report
    :param content: html content of the report

    """
    self.envReport = self._buildReport(title, description, content)

  def attach(self, report):
    """
    Adds a new latency distribution visaulization to this result object
    """
    reportcell = Reportcell(report.histogram)
    for constituent in report.constituents:
      report = self._buildReport(constituent.title, constituent.description, constituent.report)
      reportcell.htmlReport.append(report)
    self.reportCells.append(reportcell)

  def commit(self, app, profiles, reportName):
    """Commits results"""
    pass
