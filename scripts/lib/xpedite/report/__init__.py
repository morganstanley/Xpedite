# package xpedite.report
"""
Reporting package to generate html table, plots and other visualizations
This package provides modules to build reports from timeline and delta series objects

Author: Manikandan Dhamodharan, Morgan Stanley

"""
import time
import xpedite.util
import logging

from xpedite.report.env              import EnvReportBuilder
from xpedite.report.reportbuilder    import ReportBuilder

LOGGER = logging.getLogger(__name__)

class Report(object):
  """Class to store detailed report for a profiling session"""

  class Markup(object):
    """Class to store detailed latency statistics for txns with a common route"""

    def __init__(self, name, title, description, content):
      """Constructs object to store markup for profile reports"""
      self.name = name
      self.title = title
      self.description = description
      self.content = content

  class Category(object):
    """Class to store a histogram and reports for a category of txns"""

    def __init__(self, name, histogram):
      """Constructs a container object to hold histogram and reports for routes in a category"""
      self.name = name
      self.histogram = histogram
      self.routes = []

    def addRoute(self, name, title, description, content):
      """Adds a markup with detailed latency statistics"""
      self.routes.append(Report.Markup(name, title, description, content))

  def __init__(self, app, profiles, envReport, categories):
    """Constructs object to hold profile data and reports for a profiling session"""
    self.app = app
    self.profiles = profiles
    self.envReport = envReport
    self.categories = categories

  @property
  def runId(self):
    """Unique run id for this report"""
    return self.app.runId

  def makeBenchmark(self, path):
    """
    Persists samples for current run in the given path for future benchmarking

    :param path: Path to persist profiles for the current session

    """
    return self.profiles.makeBenchmark(path)

def generateEnvironmentReport(app, repo, resultOrder, classifier, txnFilter, benchmarkPaths):
  """
  Generates report with environment details

  :param app: an instance of xpedite app, to interact with target application
  :param repo: Repository of loaded transactions
  :param resultOrder: Sort order of transactions in latency constituent reports
  :param classifier: Predicate to classify transactions into different categories
  :param txnFilter: Lambda to filter transactions prior to report generation
  :param benchmarkPaths: List of stored reports from previous runs, for benchmarking

  """
  markup = EnvReportBuilder().buildEnvironmentReportFile(app, repo, resultOrder, classifier, txnFilter, benchmarkPaths)
  if markup:
    description = """
    Test environment report (cpu clock frequency, kernel configuration etc.)
    """
    title = 'Test Environment Report'
    return Report.Markup(title, title, description, markup)
  return None

def generate(app, profiles, histograms, resultOrder, classifier, txnFilter, benchmarkPaths, reportThreshold):
  """
  Generates latency breakup reports for a list of profiles

  :param app: an instance of xpedite app, to interact with target application
  :param profiles: Profile data for the current profile session
  :param histograms: Latency distribuion histograms for each category/route combination
  :param resultOrder: Sort order of transactions in latency constituent reports
  :param classifier: Predicate to classify transactions into different categories
  :param txnFilter: Lambda to filter transactions prior to report generation
  :param benchmarkPaths: List of stored reports from previous runs, for benchmarking
  :param reportThreshold: Threshold for number of transactions rendered in html reports.

  """
  envReport = generateEnvironmentReport(app, profiles.transactionRepo, resultOrder, classifier,
      txnFilter, benchmarkPaths)
  categories = {name : Report.Category(name, histogram) for name, histogram in histograms.items()}
  for profile in profiles:
    category = categories.get(profile.category, None)
    if category:
      begin = time.time()
      title = '{} latency statistics [{} transactions]'.format(profile.name, len(profile.current))
      LOGGER.info('generating report %s -> ', title)
      markup = ReportBuilder().buildReport(profile.current, profile.benchmarks, profile.reportProbes,
        profile.name, resultOrder, reportThreshold)
      markupSize = xpedite.util.formatHumanReadable(len(markup))
      title = '{} - ({})'.format(title, markupSize)
      description = '\n\t{}\n\t'.format(title)
      elapsed = time.time() - begin
      LOGGER.completed('completed %s in %0.2f sec.', markupSize, elapsed)
      category.addRoute(profile.name, title, description, markup)
  return Report(app, profiles, envReport, categories)
