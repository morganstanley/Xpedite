"""
Module to render contents of profileInfo as a html report.

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

from xpedite.report.markup     import HTML, TABLE_ENV, TD_KEY, formatList

class ProfileInfoReportBuilder(object):
  """Builds markup for rendering profile info"""

  @staticmethod
  def buildProbeTable(probes):
    """Builds a html table for the given list of probes"""
    probeTable = HTML().table(border='1', klass=TABLE_ENV)
    heading = probeTable.thead.tr
    heading.th('Probe #')
    heading.th('Probe')
    heading.th('SysName')

    tbody = probeTable.tbody
    for i, probe in enumerate(probes):
      row = tbody.tr
      row.td('{:,}'.format(i + 1), klass=TD_KEY)
      row.td('{} '.format(probe.name), klass=TD_KEY)
      row.td('{} '.format(probe.sysName))
    return probeTable

  @staticmethod
  def buildPmcTable(pmcs):
    """Builds a html table for the given list of pmu events"""
    pmcTable = HTML().table(border='1', klass=TABLE_ENV)
    heading = pmcTable.thead.tr
    heading.th('PMC #')
    heading.th('Name')
    heading.th('Uarch Name')
    heading.th('User')
    heading.th('Kernel')

    tbody = pmcTable.tbody
    for i, pmc in enumerate(pmcs):
      row = tbody.tr
      row.td('{:,}'.format(i + 1), klass=TD_KEY)
      row.td('{} '.format(pmc.name), klass=TD_KEY)
      row.td('{} '.format(pmc.uarchName))
      row.td('{} '.format(pmc.user))
      row.td('{} '.format(pmc.kernel))
    return pmcTable

  def buildReport(self, app, resultOrder, classifier, events, probes, txnFilter, benchmarkPaths):
    """Builds a html report from contents of a profile info module"""
    from xpedite.report.codeFormatter import CodeFormatter
    highlightWrapBegin = '<div class="wy-nav-content">'
    highlightWrapEnd = '</div>'
    description = """
      <div>This report provides an overview of profile info parameters used in recording this profile.</div>
      <div>The report below comprises, the list of probes, performance counters, classifiers, and other attributes that control transaction generation.</div>
    """
    report = HTML()
    report += description
    appInfoList = [
      'App Name = {}'.format(app.name),
      'App Host = {}'.format(app.ip),
      'App Info = {}'.format(app.appInfoPath),
      'xpedite Run = {}'.format(app.runId),
    ]
    if resultOrder:
      appInfoList.append('Result Order = {}'.format(resultOrder))
    report += formatList(appInfoList)
    report.h3('Probes')
    report += self.buildProbeTable(probes)
    if benchmarkPaths:
      report.h3('Benchmarks')
      report += formatList(benchmarkPaths)
    if events:
      report.h3('Performance Counters')
      report += self.buildPmcTable(events)
    if txnFilter:
      report.h3('Transaction Filter')
      report += CodeFormatter().format(txnFilter, 'highlight', highlightWrapBegin, highlightWrapEnd)
    if classifier:
      report.h3('Transaction Classifier')
      report += CodeFormatter().format(classifier, 'highlight', highlightWrapBegin, highlightWrapEnd)
    return report

  def buildProfileInfoReportFile(self, app, repo, resultOrder, classifier, txnFilter, benchmarkPaths):
    """Builds a html report from contents of a profile info module"""
    reportInfo = self.buildReport(
      app, resultOrder, classifier, repo.getCurrent().events, repo.getCurrent().probes, txnFilter, benchmarkPaths
    )
    report = str(reportInfo)
    return report
