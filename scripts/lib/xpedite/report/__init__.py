# package xpedite.report
"""
Reporting package to generate html table, plots and other visualizations
This package provides modules to build reports from timeline and delta series objects

Author: Manikandan Dhamodharan, Morgan Stanley

"""
import time
import xpedite.util
import logging

LOGGER = logging.getLogger(__name__)

class Report(object):
  """Class to store a histogram with associated constituent reports"""

  class Constituent(object):
    """Class to store detailed latency statistics for constituent reports"""

    def __init__(self, name, title, description, report):
      self.name = name
      self.title = title
      self.description = description
      self.report = report

  def __init__(self, category, histogram):
    self.category = category
    self.histogram = histogram
    self.constituents = []

  def addConstituent(self, name, title, description, report):
    """Adds a constituent report with detailed latency statistics"""
    self.constituents.append(Report.Constituent(name, title, description, report))

def generate(profiles, histograms, resultOrder, reportThreshold):
  """
  Generates latency breakup reports for a list of profiles

  :param profiles: Profile data for the current profile session
  :param histograms: Latency distribuion histograms for each category/route combination
  :param resultOrder: Sort order of transactions in latency constituent reports
  :param reportThreshold: Threshold for number of transactions rendered in html reports.

  """
  from xpedite.report.reportbuilder    import ReportBuilder
  reports = {category : Report(category, histogram) for category, histogram in histograms.iteritems()}
  for profile in profiles:
    report = reports.get(profile.category, None)
    if report:
      begin = time.time()
      reportTitle = '{} latency statistics [{} transactions]'.format(profile.name, len(profile.current))
      LOGGER.info('generating report %s -> ', reportTitle)
      constituentReport = ReportBuilder().buildReport(profile.current, profile.benchmarks, profile.reportProbes
          , profile.name, resultOrder, reportThreshold)
      reportSize = xpedite.util.formatHumanReadable(len(constituentReport))
      reportTitle = '{} - ({})'.format(reportTitle, reportSize)
      description = '\n\t{}\n\t'.format(reportTitle)
      elapsed = time.time() - begin
      LOGGER.completed('completed %s in %0.2f sec.', reportSize, elapsed)
      report.addConstituent(profile.name, reportTitle, description, constituentReport)
  return reports.values()
