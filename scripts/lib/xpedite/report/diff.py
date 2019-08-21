"""
Module to render a table comparing 2 transactions by
probe names, tsc duration, and performance counters

Author:  Brooke Elizabeth Cantwell, Morgan Stanley

"""
from thirdParty.html import HTML

class DiffBuilder(object):
  """
  DiffBuilder construcs an HTML table to be displayed in a Jupyter
  notebook comparing transaction and performance counter information
  """

  @staticmethod
  def buildDiffTitle(lhs, rhs):
    """
    Builds a title for the transaction diff table
    """
    from xpedite.report.markup import TIME_POINT_STATS_TITLE
    title = 'Transaction diff\ntxn #{} vs txn #{}'.format(lhs, rhs)
    element = HTML().div(klass=TIME_POINT_STATS_TITLE)
    element.h3(title)
    return element

  @staticmethod
  def buildDiffTableHeader(txn, table):
    """
    Builds a header for the transaction diff table

    :param txn: Timeline to construct header for
    :type txn: xpedite.analytics.timeline.TImeline
    :param table: HTML table

    """
    heading = table.thead.tr
    heading.th('Begin Probe')
    heading.th('End Probe')
    heading.th('Wall Time')

    if txn.endpoint.pmcNames:
      for pmcName in txn.endpoint.pmcNames:
        heading.th(pmcName)

    if txn.endpoint.topdownValues:
      for value in txn.endpoint.topdownValues:
        heading.th(value.name)

  def buildDiffTable(self, lhs, rhs): # pylint: disable=too-many-locals
    """
    Constructs the HTML table to show the diff of 2 transactions

    :param lhs: The timeline from conflating the two input timelines
    :type lhs: xpedite.analytics.timeline.Timeline
    :param rhs: The timeline retrieved from the second transaction ID input in the Jupyter command
    :type rhs: xpedite.analytics.timeline.Timeline
    :param profiles: Profiles build by Xpedite profiling
    :type profiles: xpedite.report.profile.Profiles

    """
    from xpedite.report.markup  import (
                                   DURATION_FORMAT, DURATION_FORMAT_2,
                                   DELTA_FORMAT_1, DELTA_FORMAT_2, TD_KEY,
                                   TABLE_SUMMARY, TRIVIAL_STATS_TABLE,
                                   getDeltaMarkup, getDeltaType
                                 )

    klass = '{} {}'.format(TABLE_SUMMARY, TRIVIAL_STATS_TABLE)
    table = HTML().table(border='1', klass=klass)

    diffReport = str(self.buildDiffTitle(lhs.txnId, rhs.txnId))
    self.buildDiffTableHeader(rhs, table)
    tbody = table.tbody

    for i in range(0, len(lhs) - 1):
      durationFmt = DURATION_FORMAT + ' ({1}' + DURATION_FORMAT_2 + ')'
      deltaFmt = DELTA_FORMAT_1 + ' ({1}' + DELTA_FORMAT_2 + ')'
      row = tbody.tr
      row.td('{}'.format(rhs[i].name, klass=TD_KEY))
      row.td('{}'.format(rhs[i + 1].name, klass=TD_KEY))

      delta = rhs[i].duration - lhs[i].duration
      row.td(durationFmt.format(
        rhs[i].duration, getDeltaMarkup(delta), delta), klass=getDeltaType(delta),
      )
      if rhs[i].deltaPmcs:
        for j, delta in enumerate(rhs[i].deltaPmcs):
          txnDelta = delta - lhs[i].deltaPmcs[j]
          row.td(deltaFmt.format(
            delta, getDeltaMarkup(txnDelta), txnDelta), klass=getDeltaType(txnDelta)
          )
      if rhs[i].topdownValues:
        for j, topdownValue in enumerate(rhs[i].topdownValues):
          delta = rhs[i].topdownValues[j].value - lhs[i].topdownValues[j].value
          row.td(durationFmt.format(
            topdownValue.value, getDeltaMarkup(delta), delta), klass=getDeltaType(delta)
          )

    diffReport += str(table)
    return diffReport
