"""
Module to build a html summary with stats, timelines and flots

This module creates a static html page with the following details
  1. Statistics tables for wall time and performance counters
  2. Latency flots at both transaction and probe level granularities
  3. Table of transactions sorted by result order

For profiles using benchmarks, the stats and flots will include
benchmark data side by side with current run.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import time
import logging
from xpedite.report.markup  import (
                              TABLE_REPORT_CONTAINER, TABLE_REPORT,
                              TABLE_ROW_NO, TABLE_ROW_DATA, TABLE_PMU,
                              TD_PMU_NAME, TD_PMU_VALUE, TH_DEBUG,
                              TD_DEBUG, TD_KEY, TD_END, DURATION_FORMAT,
                              HTML, HTML_BEGIN, HTML_END
                            )
from xpedite.util           import makeUniqueId
from xpedite.report.flot    import FlotBuilder
from xpedite.report.stats   import StatsBuilder
from xpedite.types          import ResultOrder

LOGGER = logging.getLogger(__name__)

PMU_BEGIN = """
  <script>
  $(document).ready(function () { """
PMU_BODY = """
    var uid = '{}';
    for(i=1; i<={}; i++)
      for(j=0; j<{}; j++) """
PMU_END = """
      {
        var key = '#tp'.concat('-', uid, '-', i.toString(), '-', j.toString());
        $(key).tipsy({html: true, gravity: 'sw'});
      }
  });
  </script>
"""

class ReportBuilder(object):
  """Builds latency constituent report with statistics, visualizations and timeline table"""

  @staticmethod
  def getName(probe):
    """Returns a human friendly name for the given probe"""
    return probe.name if probe.name else probe.getCanonicalName()

  @staticmethod
  def buildBreakupTableHeader(table, probes, logAbsoluteValues, logTimeline, logData):
    """
    Builds header for breakup table

    :param table: Handle to html table being rendered
    :param probes: List of probes in a transaction
    :param logAbsoluteValues: Flag to enable reporting of absolute tsc values
    :param logTimeline: Flag to enable reporting of timeline details
    :param logData: Flag to enable logging of user data associated with transaction

    """
    thead = table.thead
    heading = thead.tr
    span = '2' if logTimeline else '1'
    heading.th('No', rowspan=span, klass=TABLE_ROW_NO)
    heading.th('Transaction Id', rowspan=span, klass=TABLE_ROW_DATA)
    if logData:
      heading.th('Data', rowspan=span, klass=TABLE_ROW_DATA)
    heading.th('Time stamp (ms)', rowspan=span, klass=TABLE_ROW_DATA)

    for i, probe in enumerate(probes):
      if i < len(probes) - 1 or logTimeline:
        name = ReportBuilder.getName(probe)
        heading.th(name, colspan=span if i < len(probes) - 1 else '1')
    heading.th('Total time (us)', rowspan=span)

    if logAbsoluteValues:
      for probe in probes:
        heading.th(probe.name + ' (tsc)', rowspan=span, klass=TH_DEBUG)

    if logTimeline:
      heading = thead.tr
      for i, probe in enumerate(probes):
        heading.th('time line (us)')
        if i < len(probes) - 1:
          heading.th('duration (us)')

  @staticmethod
  def buildPmcRows(body, names, values):
    """
    Builds html table rows for each of the pmu events in a transaction

    :param body: Handle to html table object
    :param names: List of pmu event names
    :param values: List of pmu event values

    """
    for i, name in enumerate(names):
      row = body.tr
      row.td(name, klass=TD_PMU_NAME)
      row.td('{:,}'.format(values[i]), klass=TD_PMU_VALUE)
      row.newline # pylint: disable=pointless-statement

  @staticmethod
  def buildTopdownRows(body, topdownValues):
    """
    Builds html table rows for topdown metrics computed for a transaction

    :param body: Handle to html table object
    :param topdownValues: Values of computed topdown metrics

    """
    for bottleneck in topdownValues:
      row = body.tr
      row.td(bottleneck.name, klass=TD_PMU_NAME)
      row.td(DURATION_FORMAT.format(bottleneck.value), klass=TD_PMU_VALUE)
      row.newline # pylint: disable=pointless-statement

  @staticmethod
  def buildPmcTable(pmcNames, pmcValues, topdownValues):
    """
    Builds html table rows for topdown metrics and pmc values for a transaction

    :param pmcNames: List of pmu event names
    :param pmcValues: List of pmu event values
    :param topdownValues: Values of computed topdown metrics

    """
    table = HTML().table(border='0', klass=TABLE_PMU)
    heading = table.thead.tr
    heading.th('pmc')
    heading.th('value')
    body = table.tbody
    if topdownValues:
      ReportBuilder.buildTopdownRows(body, topdownValues)
    if pmcValues:
      ReportBuilder.buildPmcRows(body, pmcNames, pmcValues)
    return table

  def buildTimepointCell(self, tr, uid, xAxis, yAxis, timepoint, klass=None):
    """
    Builds html table cell for a timepoint in a timeline

    :param tr: Handle to html table row object
    :param xAxis: Identifier used to generate unique css selector
    :param yAxis: Identifier used to generate unique css selector
    :param klass: css selector for this cell (Default value = None)
    :param timepoint: Timepoint to be reported

    """
    if timepoint.pmcNames:
      title = self.buildPmcTable(timepoint.pmcNames, timepoint.deltaPmcs, timepoint.topdownValues)
      cellId = 'tp-{}-{}-{}'.format(uid, xAxis, yAxis)
      if klass:
        tr.td().a(DURATION_FORMAT.format(
          timepoint.duration), title=str(title), id=cellId, klass=klass)
      else:
        tr.td().a(DURATION_FORMAT.format(timepoint.duration), title=str(title), id=cellId)
    else:
      if klass:
        tr.td(DURATION_FORMAT.format(timepoint.duration), klass=klass)
      else:
        tr.td(DURATION_FORMAT.format(timepoint.duration))

  @staticmethod
  def buildPmuScript(timelineCollection, uid):
    """Builds javascript to display pmu metrics in timeline table"""
    timeline = timelineCollection[0]
    timepoint = timeline[0]
    if timepoint.pmcNames:
      return PMU_BEGIN + PMU_BODY.format(uid, len(timelineCollection), len(timeline)+1) + PMU_END
    return ''

  @staticmethod
  def reorderTimelineRecords(timelineCollection, resultOrder):
    """
    Reorders timelines in a collection per the given result order

    :param timelineCollection: A collection of timelines to be reordered
    :param resultOrder: Sort order for a collection of timelines

    """
    if resultOrder != ResultOrder.Chronological:
      timelineCollection = list(timelineCollection)  # make a copy, so we can sort
      if resultOrder in (ResultOrder.WorstToBest, ResultOrder.BestToWorst):
        timelineCollection = sorted(
          timelineCollection, key=lambda timeline: timeline.endpoint.duration, reverse=(
            resultOrder == ResultOrder.WorstToBest
          )
        )
      elif resultOrder == ResultOrder.TransactionId:
        timelineCollection = sorted(timelineCollection, key=lambda timeline: timeline.txnId)
    return timelineCollection

  def buildTimelineTable(self, timelineStats, probes, resultOrder, threshold, uid,
      logAbsoluteValues=False, logTimeline=False, logData=False):
    """
    Builds a html table for timelines with common category and route

    :param timelineStats: A collection of timelines to be reported
    :param probes: List of probes in route taken by, the transaction collection
    :param resultOrder: Sort order for a collection of timelines
    :param threshold: Threshold for number of transactions rendered in html reports.
    :param logAbsoluteValues: Flag to enable reporting of absolute tsc values
    :param logTimeline: Flag to enable reporting of timeline details
    :param logData: Flag to enable logging of data associated with transaction

    """
    begin = time.time()
    tableContainer = HTML().div(klass=TABLE_REPORT_CONTAINER)
    table = tableContainer.table(border='1', klass=TABLE_REPORT)
    self.buildBreakupTableHeader(table, probes, logAbsoluteValues, logTimeline, logData)
    tbody = table.tbody

    timelineCollection = self.reorderTimelineRecords(timelineStats.timelineCollection, resultOrder)

    #write table rows
    for i, timeline in enumerate(timelineCollection, 1):
      row = tbody.tr
      row.td('{0:,}'.format(i), klass=TD_KEY)
      row.td('{:,}'.format(timeline.txnId), klass=TD_KEY)
      if logData:
        row.td('{}'.format(timeline.data), klass=TD_KEY)
      row.td('{:,}'.format(timeline.inception), klass=TD_KEY)

      j = None
      for j, timepoint in enumerate(timeline):
        if logTimeline:
          row.td(DURATION_FORMAT.format(timepoint.point))
          if j < len(timeline) -1: # skip the duration for the last time point, since it's always 0
            self.buildTimepointCell(row, uid, i, j, timepoint)
        elif j < len(timeline) -1: # skip the duration for the last time point, since it's always 0
          self.buildTimepointCell(row, uid, i, j, timepoint)
      self.buildTimepointCell(row, uid, i, j, timeline.endpoint, klass=TD_END)

      if logAbsoluteValues:
        for j, probe in enumerate(probes):
          counter = timeline.txn[j]
          if probe != counter.probe:
            from xpedite.types import InvariantViloation
            raise InvariantViloation(
              'transaction {} does not match route {}'.format(timeline.txn, probes)
            )
          tsc = counter.tsc if counter else '---'
          row.td('{}'.format(tsc), klass=TD_DEBUG)
      if i >= threshold:
        break
      elapsed = time.time() - begin
      if elapsed >= 5:
        LOGGER.completed('\tprocessed %d out of %d transactions | %0.2f%% complete',
          i, threshold, float(100 * float(i)/float(threshold))
        )
        begin = time.time()
    return tableContainer

  def buildReport(self, timelineStats, benchmarkTlsMap, probes, category, resultOrder, threshold,
    logAbsoluteValues=False, logTimeline=False, logData=False):
    """
    Builds latency constituent report with statistics, visualizations and timeline table

    :param timelineStats: Time line and duration series statistics
    :type timelineStats: xpedite.analytics.timeline.TimelineStats
    :param benchmarkTlsMap: Time line and duration series statistics for benchmarks
    :param probes: List of probes in route, taken by the transaction collection
    :param category: Category of the transactions in this profile
    :param resultOrder: Sort order for a collection of timelines
    :param threshold: Threshold for number of transactions rendered in html reports
    :param logAbsoluteValues: Flag to enable reporting of absolute tsc values
    :param logTimeline: Flag to enable reporting of timeline details
    :param logData: Flag to enable logging of data associated with transaction

    """
    uid = makeUniqueId()
    tableContainer = self.buildTimelineTable(
      timelineStats, probes, resultOrder, threshold, uid, logAbsoluteValues, logTimeline, logData
    )
    timelineCollection = self.reorderTimelineRecords(timelineStats.timelineCollection, resultOrder)
    pmuScript = self.buildPmuScript(timelineCollection, uid)

    flotBuilder = FlotBuilder()
    flotMarkup = flotBuilder.buildBenchmarkFlot(category, timelineStats, benchmarkTlsMap)
    statsReport = StatsBuilder().buildStatsTable(category, timelineStats, benchmarkTlsMap)

    reportTitle = HTML().h3('{} Transaction Time lines'.format(category))

    return (HTML_BEGIN +
      statsReport +
      flotMarkup +
      str(reportTitle) +
      pmuScript +
      str(tableContainer) +
      HTML_END
    )
