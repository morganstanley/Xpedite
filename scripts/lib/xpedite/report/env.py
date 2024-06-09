"""
Module to generate profile environment report

This module creates a html report with the following details
  1. Cpu info and bios configurations
  2. Operating sytem build settings

Author: Manikandan Dhamodharan, Morgan Stanley

"""
import getpass
from time                  import gmtime, strftime
from xpedite.report.markup import TABLE_ENV, TD_KEY
from xpedite.report.markup import HTML, HTML_BEGIN, HTML_END

class EnvReportBuilder(object):
  """Builds html report with details about profiling environment"""

  @staticmethod
  def buildEnvironmentTable(app):
    """
    Builds a table with environment details

    :param app: an instance of xpedite app, to interact with target application

    """
    table = HTML().table(border='1', klass=TABLE_ENV)
    heading = table.thead.tr
    heading.th('No')
    heading.th('Parameter')
    heading.th('Value')

    details = [
      ('application', app.name),
      ('report time', strftime('%Y-%m-%d %H:%M:%S', gmtime())),
      ('host', app.ip),
      ('pid', app.pid),
      ('user', getpass.getuser()),
      ('os', app.getOsUname()),
      ('os boot param', app.getBootParam())
    ]

    tbody = table.tbody
    for i, (k, val) in enumerate(details):
      row = tbody.tr
      row.td('{:,}'.format(i+1), klass=TD_KEY)
      row.td('{} '.format(k), klass=TD_KEY)
      row.td('{} '.format(val))
    return table

  @staticmethod
  def buildCpuInfoTable(app):
    """
    Builds a table with cpu info details

    :param app: an instance of xpedite app, to interact with target application

    """
    info = app.getFullCpuInfo()
    if info:
      table = HTML().table(border='1', klass=TABLE_ENV)
      heading = table.thead.tr
      heading.th('No')
      heading.th('Info')
      heading.th('Value')

      tbody = table.tbody
      for i, (k, val) in enumerate(info.items()):
        row = tbody.tr
        row.td('{:,}'.format(i+1), klass=TD_KEY)
        row.td('{} '.format(k), klass=TD_KEY)
        row.td('{} '.format(val))
      return table
    return None

  def buildEnvironmentReportFile(self, app, repo, resultOrder, classifier, txnFilter, benchmarkPaths):
    """
    Creates a file to store the markup for environment details

    :param app: an instance of xpedite app, to interact with target application
    :param repo: repository of transactions for current profiling sessions and benchmarks
    :param resultOrder: Sort order of transactions in latency constituent reports
    :param classifier: Predicate to classify transactions into different categories
    :param txnFilter: Lambda to filter transactions prior to report generation
    :param benchmarkPaths: List of stored reports from previous runs, for benchmarking

    """
    from xpedite.report.profileInfo   import ProfileInfoReportBuilder
    from xpedite.report.tabs          import (
                                        TAB_HEADER_FMT, TAB_BODY_FMT, TAB_BODY_PREFIX, TAB_BODY_SUFFIX,
                                        TAB_JS, TAB_CONTAINER_FMT, tabState, tabContentState
                                      )
    envTable = self.buildEnvironmentTable(app)
    cpuInfoTable = self.buildCpuInfoTable(app)
    hostReport = ''
    if envTable:
      title = HTML().h3('Test Environment parameters')
      hostReport += str(title) + str(envTable)
    if cpuInfoTable:
      title = HTML().h3('Test Environment cpu info')
      hostReport += str(title) + str(cpuInfoTable)
    profileReport = ProfileInfoReportBuilder().buildProfileInfoReportFile(
      app, repo, resultOrder, classifier, txnFilter, benchmarkPaths
    )

    tabHeader = TAB_HEADER_FMT.format('hostInfo', tabState(True), 'Host Info')
    tabHeader += TAB_HEADER_FMT.format('profileInfo', tabState(False), 'Profile Info')
    envBodyClass = 'envInfoBody '
    tabBody = TAB_BODY_FMT.format('hostInfo', envBodyClass + tabContentState(True), hostReport)
    tabBody += TAB_BODY_FMT.format('profileInfo', envBodyClass + tabContentState(False), profileReport)
    tabBody = TAB_BODY_PREFIX + tabBody + TAB_BODY_SUFFIX
    report = HTML_BEGIN + TAB_CONTAINER_FMT.format(tabHeader, tabBody) + TAB_JS + HTML_END
    return report
