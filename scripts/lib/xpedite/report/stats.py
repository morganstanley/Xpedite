"""
Module to generate common statistics from a collection of txn

This module generates the following statistics at both transaction
and probe level granularities
  Min, Max, Median, Mean, 95%, 99%, Standard Deviation

In the presence of benchmarks, the stats highlight improvements or
degradation with respect to a chosen benchmark.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
from xpedite.report.markup         import (
                                     HTML, TIME_POINT_STATS_TITLE, TRIVIAL_STATS_TABLE,
                                     TABLE_SUMMARY, TIME_POINT_STATS, TD_KEY, SELECTOR,
                                     DURATION_FORMAT, DURATION_FORMAT_2
                                   )
from xpedite.util                  import makeUniqueId, loadTextFile
from xpedite.analytics.timeline    import TSC_EVENT_NAME
from xpedite.report.tabs           import (
                                     TAB_HEADER_FMT, TAB_BODY_FMT, TAB_BODY_PREFIX,
                                     TAB_BODY_SUFFIX, TAB_JS, TAB_CONTAINER_FMT,
                                     tabState, tabContentState
                                   )

class StatsBuilder(object):
  """Builds statistics for a collection of transactions sharing a category and route combination"""

  def __init__(self):
    path = os.path.dirname(__file__)
    self.benchmarkStatsContainerFmt = loadTextFile(os.path.join(path, 'benchmarkStatsContainer.fmt'))
    self.percentile1 = 95
    self.percentile2 = 99

  @staticmethod
  def buildStatsTitle(category, benchmarkNames, transactionCount):
    """
    Builds title markup for the stats table

    :param category: Category of transactions in this profile
    :param transactionCount: Number of transactions
    :param benchmarkNames: Names of given benchmarks

    """

    title = '{} latency statistics ({} transactions) {}'.format(
      category, transactionCount, ' vs benchmark - ' if benchmarkNames else ''
    )
    element = HTML().div(klass=TIME_POINT_STATS_TITLE)
    element.h3(title, style='display: inline')

    if benchmarkNames:
      bechmarkSelector = element.select(onchange='onSelectBenchmark(this)', klass=SELECTOR)
      for benchmarkName in benchmarkNames:
        bechmarkSelector.option(benchmarkName)
    return element

  def buildStatsTableHeader(self, table):
    """
    Builds header for the statistics table

    :param table: Handle to html table being rendered

    """
    heading = table.thead.tr
    heading.th('No')
    heading.th('Begin probe')
    heading.th('End probe')
    heading.th('Min')
    heading.th('Max')
    heading.th('Median')
    heading.th('Mean')
    heading.th('{}%'.format(self.percentile1))
    heading.th('{}%'.format(self.percentile2))
    heading.th('Standard Deviation')

  def buildTrivialStatsTable(self, deltaSeriesCollection, klass=TRIVIAL_STATS_TABLE, style=''):
    """
    Builds a html table with statistics for a collection of transactions

    :param deltaSeriesCollection: A series of elapsed time or pmc values for a pair of probes
    :param klass: Css selector for this table (Default value = TRIVIAL_STATS_TABLE)
    :param style: Css inline style attributes for this table (Default value = '')

    """
    tableWrapper = HTML().div()
    klass = '{} {}'.format(TABLE_SUMMARY, klass)
    table = tableWrapper.table(border='1', klass=klass, style=style)
    self.buildStatsTableHeader(table)
    tbody = table.tbody

    for i, deltaSeries in enumerate(deltaSeriesCollection, 1):
      row = tbody.tr
      row.td('{0:,}'.format(i), klass=TD_KEY)
      row.td(deltaSeries.beginProbeName, klass=TD_KEY)
      row.td(deltaSeries.endProbeName, klass=TD_KEY)
      row.td(DURATION_FORMAT.format(deltaSeries.getMin()))
      row.td(DURATION_FORMAT.format(deltaSeries.getMax()))
      row.td(DURATION_FORMAT.format(deltaSeries.getMedian()))
      row.td(DURATION_FORMAT.format(deltaSeries.getMean()))
      row.td(DURATION_FORMAT.format(deltaSeries.getPercentile(self.percentile1)))
      row.td(DURATION_FORMAT.format(deltaSeries.getPercentile(self.percentile2)))
      row.td(DURATION_FORMAT.format(deltaSeries.getStandardDeviation()))
    return tableWrapper

  def buildDifferentialStatsTable(self, deltaSeriesCollection, refDsc, klass, style):
    """
    Builds a table with statistics for current profile session side by side with benchmarks

    :param deltaSeriesCollection: A series of elapsed time or pmc values for a pair of probes
    :param refDsc: Reference delta series collection for the current profile session
    :param klass: Css selector for this table (Default value = TRIVIAL_STATS_TABLE)
    :param style: Css inline style attributes for this table (Default value = '')

    """
    from xpedite.report.markup import getDeltaMarkup, getDeltaType
    klass = '{} {}'.format(TABLE_SUMMARY, klass)
    table = HTML().table(border='1', klass=klass, style=style)
    self.buildStatsTableHeader(table)
    tbody = table.tbody
    fmt = DURATION_FORMAT + ' ({1}' + DURATION_FORMAT_2 + ')'

    for i, deltaSeries in enumerate(deltaSeriesCollection, 1):
      row = tbody.tr
      row.td('{0:,}'.format(i), klass=TD_KEY)
      row.td(deltaSeries.beginProbeName, klass=TD_KEY)
      row.td(deltaSeries.endProbeName, klass=TD_KEY)

      delta = deltaSeries.getMin() - refDsc[i-1].getMin()
      row.td(fmt.format(deltaSeries.getMin(), getDeltaMarkup(delta), delta), klass=getDeltaType(delta))

      delta = deltaSeries.getMax() - refDsc[i-1].getMax()
      row.td(fmt.format(deltaSeries.getMax(), getDeltaMarkup(delta), delta), klass=getDeltaType(delta))

      delta = deltaSeries.getMedian() - refDsc[i-1].getMedian()
      row.td(fmt.format(deltaSeries.getMedian(), getDeltaMarkup(delta), delta), klass=getDeltaType(delta))

      delta = deltaSeries.getMean() - refDsc[i-1].getMean()
      row.td(fmt.format(deltaSeries.getMean(), getDeltaMarkup(delta), delta), klass=getDeltaType(delta))

      percentile1 = deltaSeries.getPercentile(self.percentile1)
      delta = percentile1 - refDsc[i-1].getPercentile(self.percentile1)
      row.td(fmt.format(percentile1, getDeltaMarkup(delta), delta), klass=getDeltaType(delta))

      percentile2 = deltaSeries.getPercentile(self.percentile2)
      delta = percentile2 - refDsc[i-1].getPercentile(self.percentile2)
      row.td(fmt.format(percentile2, getDeltaMarkup(delta), delta), klass=getDeltaType(delta))

      delta = deltaSeries.getStandardDeviation() - refDsc[i-1].getStandardDeviation()
      row.td(fmt.format(
        deltaSeries.getStandardDeviation(), getDeltaMarkup(delta), delta), klass=getDeltaType(delta))
    return table

  def _buildStatsTable(self, eventName, deltaSeriesCollection, benchmarkTlsMap):
    """
    Builds a table with statistics for current profile session side by side with benchmarks

    :param eventName: Name of the event (Wall time or pmu event)
    :param deltaSeriesCollection: A series of elapsed time or pmc values for a pair of probes
    :param benchmarkTlsMap: Timeline statitics for benchmarks

    """
    statsReport = ''
    if benchmarkTlsMap:
      benchmarkIndex = 0
      for benchmarkTls in benchmarkTlsMap.values():
        klass = TIME_POINT_STATS.format(benchmarkIndex)
        style = 'display: {}'.format('table' if benchmarkIndex == 0 else 'none')
        refDeltaSeriesCollection = benchmarkTls.deltaSeriesRepo.get(eventName, None)
        if refDeltaSeriesCollection:
          statsReport += str(
            self.buildDifferentialStatsTable(deltaSeriesCollection, refDeltaSeriesCollection, klass, style)
          )
        else:
          statsReport += str(self.buildTrivialStatsTable(deltaSeriesCollection, klass, style))
        benchmarkIndex += 1
      statsReport = self.benchmarkStatsContainerFmt.format(statsReport)
    else:
      statsReport += str(self.buildTrivialStatsTable(deltaSeriesCollection))
    return statsReport

  def buildStatsTable(self, category, timelineStats, benchmarkTlsMap):
    """
    Builds a table with statistics for current profile session side by side with benchmarks

    :param category: Category of transactions in the given timelineStats
    :param deltaSeriesCollection: A series of elapsed time or pmc values for a pair of probes
    :param benchmarkTlsMap: Timeline statitics for benchmarks

    """
    statsReport = str(self.buildStatsTitle(category, benchmarkTlsMap.keys(), len(timelineStats)))
    if len(timelineStats.deltaSeriesRepo) > 1:
      tabHeader = ''
      tabBody = ''
      tableCount = 0
      for eventName, deltaSeriesCollection in timelineStats.deltaSeriesRepo.items():
        tabId = '{}_{}'.format(eventName, makeUniqueId())
        tabId = tabId.replace(' ', '_').replace('.', '_').replace(':', '_')
        tabHeader += TAB_HEADER_FMT.format(tabId, tabState(tableCount == 0), eventName)
        table = self._buildStatsTable(eventName, deltaSeriesCollection, benchmarkTlsMap)
        tabBody += TAB_BODY_FMT.format(tabId, tabContentState(tableCount == 0), table)
        tableCount += 1
      tabBody = TAB_BODY_PREFIX + tabBody  + TAB_BODY_SUFFIX
      statsReport += TAB_CONTAINER_FMT.format(tabHeader, tabBody) + TAB_JS
    else:
      deltaSeriesCollection = timelineStats.getTscDeltaSeriesCollection()
      statsReport += self._buildStatsTable(TSC_EVENT_NAME, deltaSeriesCollection, benchmarkTlsMap)
    return statsReport
