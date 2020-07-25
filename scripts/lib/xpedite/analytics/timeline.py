"""
Module to build timelines, delta series from aggregated transactions

This module provides the foundation types needed for real time analytics.
It also includes logic to compute timeline statistics from aggreagated transactions

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import time
import numpy
import logging
from collections           import OrderedDict
from xpedite.types.probe   import compareProbes
from xpedite.types.route   import conflateRoutes

LOGGER = logging.getLogger(__name__)

TSC_EVENT_NAME = 'wall time'

class Timeline(object):
  """A timeline is a sequence of events happening as time progresses"""

  def __init__(self, txn):
    """
    Creates an instance of Timeline for the given transaction

    :param txn: Source transaction for this timeline
    :type data: xpedite.transaction.Transaction
    """
    self.txn = txn
    self.tsc = txn[0].tsc
    self.txnId = txn.txnId
    self.points = []
    self.endpoint = None
    self.inception = None

  def addTimePoint(self, timePoint):
    """
    Adds a time point to this time line

    :param timePoint: A time point for an event hapenning at a specific point in time
    :type timePoint: xpedite.analytics.timeline.TimePoint

    """
    self.points.append(timePoint)

  @property
  def duration(self):
    """Elapsed wall time (in micro seconds) for this timeline"""
    return self.endpoint.duration

  def __getitem__(self, index):
    """Returns a time point at a given index in this time line"""
    return self.points[index]

  def __len__(self):
    """
    Returns the length of this time line.

    The length of a time line counts the number of timepoints in the line

    """
    return len(self.points)

  def __repr__(self):
    """Returns str representation of a timeline"""
    pointStr = '\n\t'.join((str(point) for point in self.points))
    return 'Timeline: id {} | ({})\n\t'.format(self.txnId, pointStr)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class TimePoint(object):
  """A time point marks a specific instance of time in a time line"""

  def __init__(self, name, point=None, duration=None, pmcNames=None, deltaPmcs=None, topdownValues=None, data=None):
    """
    Creates an instance of TimePoint

    :param name: The name of this time point
    :type name: str
    :param point: The absolute point in time, when an event occurred
    :type point: double
    :param duration: The total duration (in micro seconds) spanned by this time point
    :type duration: double
    :param pmcNames: The list of pmu event names captured by this timepoint
    :param deltaPmcs: The list of pmu event values captured by this timepoint
    :param topdownValues: The list of topdown values computed for this timepoint
    :param data: The 128 bit raw data captured by this timepoint

    """
    self.name = name
    self.point = point
    self.duration = duration
    self.pmcNames = pmcNames
    self.deltaPmcs = deltaPmcs
    self.topdownValues = topdownValues
    self.data = data

  def __repr__(self):
    """Returns str representation of a TimePoint"""
    rep = 'TimePoint {0}: point {1:4,.3f} | duration {2:4,.3f}'.format(self.name, self.point, self.duration)
    if self.deltaPmcs:
      rep += ' | pmc {}'.format({self.pmcNames[i]: self.deltaPmcs[i] for i in range(len(self.deltaPmcs))})
    return rep

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class DeltaSeries(object):
  """A series of duration (micro seconds) and pmu counter values"""

  def __init__(self, beginProbeName, endProbeName):
    """
    Creates an instance of Duration Series

    A duration is a measure of time or pmu events, expended to execute code between
    a pair of probes

    :param beginProbeName: Name of the probe, that marks the beginning of this time period
    :type beginProbeName: str
    :param endProbeName: Name of the probe, that marks the end of this time period
    :type endProbeName: str

    """
    self.beginProbeName = beginProbeName
    self.endProbeName = endProbeName
    self.series = []
    self._count = 0
    self._min = None
    self._max = None
    self._median = None
    self._mean = None
    self._standardDeviation = None
    self.numpyArray = None

  def _computeStats(self):
    """Computes statistics for a series of druation/counter values"""
    if self.series and self._count != len(self.series):
      self._count = len(self.series)
      self._min = min(self.series)
      self._max = max(self.series)
      self._median = numpy.median(self.series)
      self._mean = numpy.mean(self.series)
      self._standardDeviation = numpy.std(self.series)
      self.numpyArray = numpy.array(self.series)

  def getStats(self):
    """Returns the underlying numpy array for this delta series"""
    self._computeStats()
    return self.numpyArray

  def getCount(self):
    """Returns the count of values in this delta series"""
    self._computeStats()
    return self._count

  def getMin(self):
    """Returns the minimum value in this delta series"""
    self._computeStats()
    return self._min

  def getMax(self):
    """Returns the maximum value in this delta series"""
    self._computeStats()
    return self._max

  def getMedian(self):
    """Returns the median value of this delta series"""
    self._computeStats()
    return self._median

  def getMean(self):
    """Returns the mean value of this delta series"""
    self._computeStats()
    return self._mean

  def getPercentile(self, percentile):
    """
    Returns value at the given percentile in this delta series

    :param percentile: Percentile to extract

    """
    self._computeStats()
    return numpy.percentile(self.numpyArray, percentile)

  def getStandardDeviation(self):
    """Returns the standard deviation value of this delta series"""
    self._computeStats()
    return self._standardDeviation

  def addDelta(self, delta):
    """
    Adds a time duration/counter value to this series

    :param delta: A span of time or pmc counter value to add to this series
    :type delta: C{double}

    """
    self.series.append(delta)

  def __len__(self):
    """Returns the length of this delta Series"""
    return len(self.series)

  def __getitem__(self, index):
    """Returns value at a given index in this series"""
    return self.series[index]

  def __repr__(self):
    """Returns str representation of this delta Series"""
    return 'Duration Series [{} -> {}]: {} elements'.format(self.beginProbeName, self.endProbeName, len(self.series))

  def __eq__(self, other):
    return self.numpyArray.all() == other.numpyArray.all()

class DeltaSeriesCollection(object):
  """A collection of delta series objects"""

  def __init__(self, eventName):
    self.eventName = eventName
    self.deltaSeriesList = []

  def addDeltaSeries(self, deltaSeries):
    """
    Adds a delta series to this collection

    :param deltaSeries: Delta series to be added

    """
    self.deltaSeriesList.append(deltaSeries)

  def __len__(self):
    """Returns the count of delta series in this collection"""
    return len(self.deltaSeriesList)

  def __getitem__(self, index):
    """Returns a delta series at a given index in this series"""
    return self.deltaSeriesList[index]

  def __repr__(self):
    """Returns str representation of this delta series collection"""
    durationSeriesCollectionStr = '{'
    for series in self.deltaSeriesList:
      durationSeriesCollectionStr += '\n\t{}'.format(series)
    durationSeriesCollectionStr += '\n}'
    return durationSeriesCollectionStr

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class DeltaSeriesRepo(object):
  """A Repository of delta series collection objects"""

  def __init__(self, events, topdownKeys, probes):
    self.deltaSeriesCollectionMap = OrderedDict([(TSC_EVENT_NAME, DeltaSeriesCollection(TSC_EVENT_NAME))])
    self.events = events
    self.pmcNames = [event.name for event in events] if events else []
    self.topdownKeys = topdownKeys
    topdownNames = []
    for key in self.topdownKeys:
      key.visit(topdownNames)
    self.eventNames = self.pmcNames + topdownNames
    if self.eventNames:
      for eventName in self.eventNames:
        self.deltaSeriesCollectionMap.update({eventName:DeltaSeriesCollection(eventName)})

    for deltaSeriesCollection in self.deltaSeriesCollectionMap.values():
      for i in range(1, len(probes)):
        deltaSeriesCollection.addDeltaSeries(DeltaSeries(probes[i-1].name, probes[i].name))
      deltaSeriesCollection.addDeltaSeries(DeltaSeries('Begin', 'End'))


  def addDeltaSeriesCollection(self, deltaSeriesCollection):
    """
    Adds a delta series collection to this repository

    :param deltaSeriesCollection: deltaSeriesCollection to be added

    """
    self.deltaSeriesCollectionMap.update({deltaSeriesCollection.eventName: deltaSeriesCollection})

  def getTscDeltaSeriesCollection(self):
    """Returns delta series collection for cpu timestamp counter"""
    return self.deltaSeriesCollectionMap[TSC_EVENT_NAME]

  def buildEventsMap(self):
    """Returns a map of event names to index for computation of topdown metrics"""
    return {event.uarchName : i for i, event in enumerate(self.events)} if self.events else None

  def __len__(self):
    """Returns count of delta series collections in this repository"""
    return len(self.deltaSeriesCollectionMap)

  def get(self, eventName, defaultValue):
    """
    Returns delta series collection for the given pmu event

    :param eventName: Name of the pmu event
    :param defaultValue: Default value for missing collections

    """
    return self.deltaSeriesCollectionMap.get(eventName, defaultValue)

  def __getitem__(self, eventName):
    """Returns a duration collection for a given event"""
    return self.deltaSeriesCollectionMap[eventName]

  def items(self):
    """Returns iterable items from the underlying container"""
    return self.deltaSeriesCollectionMap.items()

  def isEventsEnabled(self):
    """Checks if this repository stores data for PMU events"""
    return self.eventNames and len(self.eventNames) > 0

  def __repr__(self):
    """Returns str representation of this Duration Series Repo"""
    durationSeriesRepoStr = ''
    for name, deltaSeriesCollection in self.deltaSeriesCollectionMap.items():
      durationSeriesRepoStr += '{}{}\n'.format(name, deltaSeriesCollection)
    return 'Duration Series Repo \n pmc {}\ntopdown {}\n{}'.format(
      self.pmcNames, self.topdownKeys, durationSeriesRepoStr
    )

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class TimelineStats(object):
  """A container for various timeline related statistics data"""

  def __init__(self, name, cpuInfo, category, route, reportProbes, timelineCollection, deltaSeriesRepo):
    """
    Constructs an instance of TimelineStats

    :param category: Category of the transacations in this timeline stats
    :type category: str
    :param route: Route taken by the transactions in this timeline stats
    :type route: xpedite.types.route.Route
    :param timelineCollection: A collection of timelines for current profile session or a benchmark
    :param deltaSeriesRepo: A repository of delta series collections
    :type deltaSeriesRepo: xpedite.analytics.timeline.DeltaSeriesRepo
    """

    self.name = name
    self.cpuInfo = cpuInfo
    self.category = category
    self.route = route
    self.reportProbes = reportProbes
    self.timelineCollection = timelineCollection
    self.deltaSeriesRepo = deltaSeriesRepo

  @property
  def probes(self):
    """Returns probes in the route taken by the timelines in this object"""
    return self.route.probes

  @property
  def events(self):
    """Returns pmu event values captured by timelines in this object"""
    return self.deltaSeriesRepo.events

  @property
  def pmcNames(self):
    """Returns pmu event names captured by timelines in this object"""
    return self.deltaSeriesRepo.pmcNames

  @property
  def topdownKeys(self):
    """Returns topdown keys computed for timelines in this object"""
    return self.deltaSeriesRepo.topdownKeys

  @property
  def eventNames(self):
    """Returns pmu event names captured by timelines in this object"""
    return self.deltaSeriesRepo.eventNames

  def buildEventsMap(self):
    """Returns a map of event names to index for computation of topdown metrics"""
    return self.deltaSeriesRepo.buildEventsMap()

  def isEventsEnabled(self):
    """Checks, if this timeline stats stores data for any PMU events"""
    return self.deltaSeriesRepo.isEventsEnabled()

  def getTscDeltaSeriesCollection(self):
    """Returns delta series collection for cpu timestamp counter"""
    return self.deltaSeriesRepo.getTscDeltaSeriesCollection()

  def getTotalDurationSeries(self):
    """Returns the delta series for total elapsed time"""
    return self.getTscDeltaSeriesCollection()[-1] if self.getTscDeltaSeriesCollection() else None

  def __len__(self):
    return len(self.timelineCollection)

  def __repr__(self):
    return 'TimelineStats for {}\n\tcpu - {}\n\tcatetory -{}\n\t route {} \n\ttime lines {}\n\n\t{}'.format(
      self.name, self.cpuInfo, self.category, self.route, len(self.timelineCollection), self.deltaSeriesRepo
    )

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

  def __getitem__(self, index):
    return self.timelineCollection[index]

class CounterMap(object):
  """A event to index map for counters in a timepoint"""

  def __init__(self, eventsMap, counters):
    self.eventsMap = eventsMap
    self.counters = counters

  def __contains__(self, eventName):
    return eventName in self.eventsMap

  def __getitem__(self, eventName):
    index = self.eventsMap[eventName]
    return float(self.counters[index])

  def __len__(self):
    return len(self.eventsMap)

  def __repr__(self):
    return '{} | {}'.format(self.eventsMap, self.counters)


NAN = float('nan')

def buildTimelineStats(category, route, probes, txnSubCollection): # pylint: disable=too-many-locals
  """
  Builds timeline statistics from a subcollection of transactions

  :param probes: List of probes enabled for a profiling session
  :param txnSubCollection: A subcollection of transactions

  """
  from xpedite.types import InvariantViloation
  begin = time.time()
  cpuInfo = txnSubCollection.cpuInfo
  topdownMetrics = txnSubCollection.topdownMetrics
  timelineCollection = []
  topdownKeys = topdownMetrics.topdownKeys() if topdownMetrics else []
  deltaSeriesRepo = DeltaSeriesRepo(txnSubCollection.events, topdownKeys, probes)
  pmcNames = deltaSeriesRepo.pmcNames
  eventsMap = deltaSeriesRepo.buildEventsMap()
  timelineStats = TimelineStats(
    txnSubCollection.name, cpuInfo, category, route,
    probes, timelineCollection, deltaSeriesRepo
  )
  tscDeltaSeriesCollection = deltaSeriesRepo.getTscDeltaSeriesCollection()

  pmcCount = len(txnSubCollection.events) if txnSubCollection.events else 0
  inceptionTsc = None
  defaultIndices = range(len(route))

  totalTxnCount = len(txnSubCollection)
  for txnCount, txn in enumerate(txnSubCollection):
    timeline = Timeline(txn)
    indices = conflateRoutes(txn.route, route) if len(txn) > len(route) else defaultIndices
    firstCounter = prevCounter = None
    maxTsc = 0
    i = -1
    endpoint = TimePoint('end', 0, deltaPmcs=([0]* pmcCount if pmcCount > 0 else None))
    for j in indices:
      i += 1
      probe = probes[i]
      counter = txn[j]
      if not compareProbes(probe, counter.probe):
        raise InvariantViloation('category [{}] has mismatch of probes '
          '"{}" vs "{}" in \n\ttransaction {}]\n\troute {}'.format(
            category, probe, counter.probe, txn.txnId, probes
          )
        )

      if counter:
        tsc = counter.tsc
        maxTsc = max(maxTsc, tsc)
        if not firstCounter:
          firstCounter = prevCounter = counter
        elif tsc:
          duration = cpuInfo.convertCyclesToTime(tsc - prevCounter.tsc)
          point = cpuInfo.convertCyclesToTime(prevCounter.tsc - firstCounter.tsc)
          timePoint = TimePoint(probes[i-1].name, point, duration, data=prevCounter.data)

          if len(counter.pmcs) < pmcCount:
            raise InvariantViloation(
              'category [{}] has transaction {} with counter {} '
              'missing pmc samples {}/{}'.format(
                category, txn.txnId, counter, len(counter.pmcs), pmcCount
              )
            )
          if pmcCount != 0:
            timePoint.pmcNames = pmcNames
            timePoint.deltaPmcs = []
            for k in range(pmcCount):
              deltaPmc = counter.pmcs[k] - prevCounter.pmcs[k] if counter.threadId == prevCounter.threadId  else NAN
              endpoint.deltaPmcs[k] += (deltaPmc if counter.threadId == prevCounter.threadId else 0)
              timePoint.deltaPmcs.append(deltaPmc)
              deltaSeriesRepo[pmcNames[k]][i-1].addDelta(deltaPmc)
            if topdownMetrics:
              counterMap = CounterMap(eventsMap, timePoint.deltaPmcs)
              timePoint.topdownValues = topdownMetrics.compute(counterMap)
              for td in timePoint.topdownValues:
                deltaSeriesRepo[td.name][i-1].addDelta(td.value)
          timeline.addTimePoint(timePoint)
          tscDeltaSeriesCollection[i-1].addDelta(duration)
          prevCounter = counter
        else:
          raise InvariantViloation(
            'category [{}] has transaction {} with missing tsc for probe {}/counter {}'.format(
              category, txn.txnId, probe, counter
            )
          )
      else:
        raise InvariantViloation(
          'category [{}] has transaction {} with probe {} missing counter data'.format(
            category, probe, txn.txnId
          )
        )

    if prevCounter:
      point = cpuInfo.convertCyclesToTime(prevCounter.tsc - firstCounter.tsc)
      timeline.addTimePoint(TimePoint(probes[-1].name, point, 0, data=prevCounter.data))

    endpoint.duration = cpuInfo.convertCyclesToTime(maxTsc - firstCounter.tsc)
    if pmcCount != 0:
      endpoint.pmcNames = pmcNames
      for k, deltaPmc in enumerate(endpoint.deltaPmcs):
        deltaSeriesRepo[pmcNames[k]][-1].addDelta(deltaPmc)
      if topdownMetrics:
        counterMap = CounterMap(eventsMap, endpoint.deltaPmcs)
        endpoint.topdownValues = topdownMetrics.compute(counterMap)
        for td in endpoint.topdownValues:
          deltaSeriesRepo[td.name][-1].addDelta(td.value)
    timeline.endpoint = endpoint

    timelineCollection.append(timeline)
    tscDeltaSeriesCollection[-1].addDelta(endpoint.duration)

    elapsed = time.time() - begin
    if elapsed >= 5:
      LOGGER.completed(
        '\tprocessed %d out of %d transactions | %0.2f%% complete |',
        txnCount, totalTxnCount, float(100 * float(txnCount)/float(totalTxnCount))
      )
      begin = time.time()

    if not inceptionTsc:
      inceptionTsc = firstCounter.tsc
      timeline.inception = 0
    else:
      timeline.inception = int(cpuInfo.convertCyclesToTime(firstCounter.tsc - inceptionTsc) / 1000)

  return timelineStats
