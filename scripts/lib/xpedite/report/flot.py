"""
Module to generate visiualization for a collection of transactions.

This module creates plots to visualize
  1. End to end transaction latency in chronological order
  2. Latency for each section of code encapsulated by a probe pair

Transactions for benchmarks are plotted side by side with current run.

For profiles with performance counters, the module generates additional plot
with all the performance couters.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.report.markup    import HTML, TIME_POINT_STATS_TITLE, SELECTOR
from xpedite.util             import makeUniqueId
import json

FLOT_JS_BEGIN_FMT = """
<script>
  var {}SeriesCollection;
  $(document).ready(function () {{
"""
FLOT_JS_BODY_FMT = """
  {1}SeriesCollection = {0};
  var _sc = {1}SeriesCollection;
  var placeholderId = "#{1}FlotPlaceholder";
  var constituentSelectorId = "#{1}ConstituentSelector";
  var choiceContainerId = "#{1}FlotChoiceContainer";
"""
FLOT_JS_END = """
    var choiceContainer = $(choiceContainerId);
    var constituentSelector = $(constituentSelectorId);
    $.each(_sc[0], function(key, val) {
        choiceContainer.append("<p class='flotChoice'><input type='checkbox' name='" + key +
          "' checked='checked' id='id" + key + "'></input>" +
          "<label for='id" + key + "'>" +
          val.label + "</label></p>");
      });
    choiceContainer.find("input").click(
      {
        seriesCollection: _sc,
        constituentSelectorId: constituentSelectorId,
        placeholderId: placeholderId,
        choiceContainerId: choiceContainerId
      },
      onSelectSeries
    );
    constituentSelector.click(
      {
        seriesCollection: _sc,
        constituentSelectorId: constituentSelectorId,
        placeholderId: placeholderId,
        choiceContainerId: choiceContainerId
      },
      onSelectSeries
    );
    plotAccordingToChoices(_sc[_sc.length -1], placeholderId, choiceContainerId);
  });
</script>
"""
FLOT_CHOICE_BLOCK_FMT = """
<div id="{0}FlotContainer" class="flotContainer">
  <div id="{0}FlotPlaceholder" class="flotPlaceholder"> </div>
  <div id="{0}FlotChoiceContainer" class="flotChoiceContainer">
    <span>This chart plots the constituent latency of all the transactions for current run side by side with {1}s.
    The x-axis represents the transaction id and y-axis represents the constituent latency in Micro Seconds.
    select the curves to be plotted.</span>
  </div>
</div>
"""

class FlotBuilder(object):
  """
  Builds chart visualization for a delta series collections from current profile session and benchmarks

  The flot builder creates charts with txnId in x-axis and duration (micro seconds) in y-axis
  The builder creates a line chart for delta series, from each pair of probes in the timeline

  """

  @staticmethod
  def buildFlotSeriesMap(series, uid):
    """
    Builds a map with data and options for creating visualizations

    :param series: the collection of series to be plotted

    """
    seriesMap = {}
    for index, _ in enumerate(series):
      name, serie = series[index]
      seriesId = '{}_{}'.format(name, uid)
      seriesMap.update(
        {
          seriesId :
          {
            'label': name,
            'data': list(zip(range(0, len(serie)), serie)),
            'points': {'show' : True},
            'lines': {'show' : True}
          }
        }
      )
    return seriesMap

  @staticmethod
  def buildFlotTitle(category, title, timelineStats, uid):
    """
    Builds markup to render title for txn visualizations

    :param category: Category of transactions visualized by this flot
    :param title: Text title for this visualization
    :param timelineStats: Timeline stats with delta series to be plotted
    :param uid: Unique identifier to generate css selector id

    """
    constituentNames = ['{} -> {}'.format(deltaSeries.beginProbeName,
      deltaSeries.endProbeName) for deltaSeries in timelineStats.getTscDeltaSeriesCollection()]

    title = '{} {} charts {}'.format(category, title, ' for constituent - ' if constituentNames else '')
    element = HTML().div(klass=TIME_POINT_STATS_TITLE)
    element.h3(title, style='display: inline')

    if constituentNames:
      elementId = '{}ConstituentSelector'.format(uid)
      constituentSelector = element.select(id=elementId, klass=SELECTOR)
      for i, constituentName in enumerate(constituentNames):
        if i == len(constituentNames) -1:
          constituentSelector.option(constituentName, selected='selected')
        else:
          constituentSelector.option(constituentName)
    return element

  def buildFlot(self, category, title, timelineStats, uid, flotData, flotChoiceName=None):
    """
    Builds line charts for elapsed duration/pmc data from each pair of probes in the timelines

    :param category: Category of transactions visualized by this flot
    :param title: Title for the visualization
    :param timelineStats: Timeline stats with delta series to be plotted
    :param uid: Unique identifier to generate css selector id
    :param flotData: series to be plotted
    :param flotChoiceName: Name used to generate unique css selectors (Default value = None)

    """
    flotTitle = str(self.buildFlotTitle(category, title, timelineStats, uid))
    flotJsBegin = FLOT_JS_BEGIN_FMT.format(uid)
    flotBody = FLOT_JS_BODY_FMT.format(json.dumps(flotData), uid)
    flotChoiceBlock = FLOT_CHOICE_BLOCK_FMT.format(uid, flotChoiceName if flotChoiceName else uid)
    return flotTitle + flotJsBegin + flotBody + FLOT_JS_END + flotChoiceBlock

  def buildBenchmarkFlot(self, category, timelineStats, benchmarkTlsMap):
    """
    Builds line charts for data from current profile session side by side with benchmarks

    :param category: Category of transactions visualized by this flot
    :param timelineStats: Timeline stats with delta series to be plotted
    :param benchmarkTlsMap: Timeline stats for all the loaded benchmarks

    """
    uid = 'benchmark_{}'.format(makeUniqueId())
    flotData = []
    tscDeltaSeriesCollection = timelineStats.getTscDeltaSeriesCollection()
    for i, _ in enumerate(tscDeltaSeriesCollection):
      series = [(timelineStats.name, tscDeltaSeriesCollection[i])]
      for benchmarkName, benchmarkTls in benchmarkTlsMap.items():
        series.append((benchmarkName, benchmarkTls.getTscDeltaSeriesCollection()[i]))
      seriesMap = FlotBuilder.buildFlotSeriesMap(series, uid)
      flotData.append(seriesMap)
    flot = self.buildFlot(category, 'Transaction latency', timelineStats, uid, flotData)
    if timelineStats.isEventsEnabled():
      flot += self.buildPMUFlot(category, timelineStats)
    return flot

  def buildPMUFlot(self, category, timelineStats):
    """
    Builds line charts for pmc data from current profile session

    :param category: Category of transactions visualized by this flot
    :param timelineStats: Timeline stats with delta series to be plotted

    """
    uid = 'pmu_{}'.format(makeUniqueId())
    flotData = []
    tscDeltaSeriesCollection = timelineStats.getTscDeltaSeriesCollection()
    for i, _ in enumerate(tscDeltaSeriesCollection):
      series = [('wall time(us)', tscDeltaSeriesCollection[i])]
      deltaSeriesRepo = timelineStats.deltaSeriesRepo
      for eventName in deltaSeriesRepo.eventNames:
        series.append((eventName, deltaSeriesRepo[eventName][i]))
      seriesMap = FlotBuilder.buildFlotSeriesMap(series, uid)
      flotData.append(seriesMap)
    return self.buildFlot(category, 'PMU Counters', timelineStats, uid, flotData, flotChoiceName='pmu counter')
