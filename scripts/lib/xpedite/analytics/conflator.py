"""
Module to conflate timelines, based on route

Conflator conflates (combines) points in timelines to make them aggregatable with a source route.

The timelines with route, that are super set of a given source route, have additional
time points. The conflator combines the extraneous points to build new timelines, that
are one to one match for the given source route.

Given a list of source profiles and a source route, the conflator creates a new profile by
with conflated timelines from source profiles.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import math
from xpedite.analytics.timeline    import (
                                     DeltaSeriesRepo, Timeline, TimePoint,
                                     TimelineStats, CounterMap
                                   )

from xpedite.profiler.profile      import Profile
from xpedite.pmu.event             import TopdownMetrics
from xpedite.types.route           import conflateRoutes

class Conflator(object):
  """
  Aggregates transactions in one or more source profiles to one destination profile

  Transactions to a given route are conflated and
  aggreaged to build a new profile instance.
  """

  def __init__(self):
    self.eventsDbRepo = {}
    self.topdownRepo = {}

  def getEventsDb(self, cpuId):
    """
    Loads and caches PMU events database for the given cpu id

    :param cpuId: target cpu id for eventsDb

    """
    from xpedite.pmu.eventsDb import loadEventsDb
    if cpuId not in self.eventsDbRepo:
      self.eventsDbRepo.update({cpuId : loadEventsDb(cpuId)})
    return self.eventsDbRepo[cpuId]

  def getTopdown(self, cpuId):
    """
    Builds and caches topdown hierarchy for the target cpu

    :param cpuId: target cpu id for topdown hierarchy

    """
    if cpuId not in self.topdownRepo:
      eventsDb = self.getEventsDb(cpuId)
      from xpedite.pmu.topdown import Topdown
      self.topdownRepo.update({cpuId : Topdown(eventsDb)})
    return self.topdownRepo[cpuId]

  def getTopdownMetrics(self, cpuId, topdownKeys):
    """
    Builds topdown metrics for the given cpu and topdown keys

    :param cpuId: Id of target cpu used to build topdown metrics
    :param topdownKeys: Nodes or metrics in top down hierarchy

    """
    if topdownKeys:
      topdown = self.getTopdown(cpuId)
      topdownMetrics = TopdownMetrics()
      for key in topdownKeys:
        topdownMetrics.add(topdown, key)
      return topdownMetrics
    return None

  @staticmethod
  def copyProfileLayout(srcProfile, category, route):
    """
    Creates a new Profile, cloning layout from a source profile

    :param srcProfile: Source profile to clone
    :param route: Route of transactions in the new profile
    :param category: Category of transactions in the new profile

    """
    profile = Profile(
      srcProfile.name, current=Conflator.createTimelineStats(srcProfile.current, category, route), benchmarks={}
    )
    return profile

  def conflateProfiles(self, profiles, route, category):
    """
    Conflates multiple profiles into one

    Transactions across profiles, that are conflatable with given route
    are conflated and aggreagated into a new destination profile

    :param profiles: Profiles to conflate
    :param route: Route to use for conflating transactions
    :param category: Transaction category for destination profile

    """
    dst = self.copyProfileLayout(profiles[0], category, route)
    for src in profiles:
      self.conflateTimelineStats(route, src.current, dst.current)
      for name, benchmarkTLS in src.benchmarks.items():
        dstBenchmarkTLS = dst.benchmarks[name] if name in dst.benchmarks else Conflator.createTimelineStats(
          benchmarkTLS, category, route
        )
        if self.conflateTimelineStats(route, benchmarkTLS, dstBenchmarkTLS):
          dst.benchmarks.update({name: dstBenchmarkTLS})

    dst.current.timelineCollection.sort(key=lambda timeline: timeline.tsc)
    Conflator.buildDeltaSeriesRepo(dst.current)
    for benchmarkTLS in dst.benchmarks.values():
      benchmarkTLS.timelineCollection.sort(key=lambda timeline: timeline.tsc)
      Conflator.buildDeltaSeriesRepo(benchmarkTLS)
    return dst

  def conflateTimelineStats(self, route, src, dst):
    """
    Conflates timelines from source timeline stats to destination timeline stats

    :param route: route used for conflation
    :param src: Source timeline stats
    :param dst: Destination timeline stats

    """
    routeIndices = conflateRoutes(src.route, route)
    if routeIndices:
      topdownMetrics = self.getTopdownMetrics(src.cpuInfo.cpuId, src.topdownKeys)
      self.conflateTimelineCollection(
        routeIndices, src.timelineCollection, dst.timelineCollection,
        src.buildEventsMap(), topdownMetrics
      )
      return True
    return None

  def conflateTimelineCollection(self, routeIndices, src, dst, eventsMap, topdownMetrics):
    """
    Conflates timelines from source timeline collection to destination timeline collection

    :param routeIndices: Subset of route indices for conflation
    :param src: Source timeline collection
    :param dst: Destination timeline collection
    :param topdownMetrics: Topdown metrics for the conflated timelines
    :param eventsMap: Map of pmu events for compution of topdown metrics

    """
    for timeline in src:
      conflatedTl = self.conflateTimeline(timeline, routeIndices, eventsMap, topdownMetrics)
      dst.append(conflatedTl)

  def conflateTimeline(self, srcTl, routeIndices, eventsMap, topdownMetrics):
    """
    Constructs a conflated timeline from source timeline

    :param srcTl: Source timeline to conflate
    :param routeIndices: Subset of route indices for conflation
    :param eventsMap: Map of pmu events for compution of topdown metrics
    :param topdownMetrics: Topdown metrics for the conflated timelines

    """
    timeline = Timeline(srcTl.txn)
    timeline.inception = srcTl.inception
    timeline.points = self.conflateTimepoints(srcTl.points, routeIndices, eventsMap, topdownMetrics)
    timeline.endpoint = self.conflateTimepoints(
      timeline.points, [0, len(timeline.points) -1],
      eventsMap, topdownMetrics
    )[0]
    return timeline

  @staticmethod
  def conflateTimepoints(srcTimePoints, routeIndices, eventsMap, topdownMetrics):
    """
    Constructs a list of conflated timepoints from a given list of source timepoints

    :param srcTimePoints: A list of source time points to conflate
    :param routeIndices: Subset of route indices for conflation
    :param eventsMap: Map of pmu events for compution of topdown metrics
    :param topdownMetrics: Topdown metrics for the conflated timepoints

    """
    timePoints = []
    for i, _ in enumerate(routeIndices):
      begin = routeIndices[i]
      end = (begin + 1) if(i == len(routeIndices)-1) else routeIndices[i + 1]
      srcTp = srcTimePoints[begin]
      dstTp = TimePoint(
        srcTp.name, point=srcTp.point, duration=srcTp.duration, pmcNames=srcTp.pmcNames,
        deltaPmcs=list(srcTp.deltaPmcs) if srcTp.deltaPmcs else None,
      )
      for j in range(begin+1, end):
        srcTp = srcTimePoints[j]
        dstTp.duration += srcTp.duration
        if srcTp.deltaPmcs:
          for k, _ in enumerate(srcTp.deltaPmcs):
            if not math.isnan(srcTp.deltaPmcs[k]):
              dstTp.deltaPmcs[k] += srcTp.deltaPmcs[k]
      if topdownMetrics and dstTp.deltaPmcs:
        counterMap = CounterMap(eventsMap, dstTp.deltaPmcs)
        dstTp.topdownValues = topdownMetrics.compute(counterMap)
      timePoints.append(dstTp)
    return timePoints

  @staticmethod
  def createTimelineStats(src, category, route):
    """
    Constructs an empty timeline stats cloning meta data from source

    :param src: Source timeline stats used for cloning meta data
    :param category: Category for transactions in conflated profile
    :param route: Route to use for conflating transactions

    """
    deltaSeriesRepo = DeltaSeriesRepo(src.events, src.topdownKeys, route.probes)
    return TimelineStats(src.name, src.cpuInfo, category, route, route.probes, [], deltaSeriesRepo)

  @staticmethod
  def addTimepoint(deltaSeriesRepo, timepoint, index):
    """
    Adds tsc and pmc data from the given time point to delta series repository

    :param deltaSeriesRepo: Delta series repository to be enriched
    :param index: Index in delta series repo to enrich
    :param timepoint: Time point to be added

    """
    from xpedite.analytics.timeline import TSC_EVENT_NAME
    deltaSeriesRepo[TSC_EVENT_NAME][index].addDelta(timepoint.duration)
    if timepoint.pmcNames:
      for j, pmcName in enumerate(timepoint.pmcNames):
        deltaSeriesRepo[pmcName][index].addDelta(timepoint.deltaPmcs[j])
    if timepoint.topdownValues:
      for j, topdownValue in enumerate(timepoint.topdownValues):
        deltaSeriesRepo[topdownValue.name][index].addDelta(topdownValue.value)

  @staticmethod
  def buildDeltaSeriesRepo(timelineStats):
    """
    Builds a new delta series repository from timelines in the given source timeline stats

    :param timelineStats: Source timeline stats used to build delta series repository

    """
    deltaSeriesRepo = timelineStats.deltaSeriesRepo
    for timeline in timelineStats.timelineCollection:
      for i in range(len(timeline) -1):
        Conflator.addTimepoint(deltaSeriesRepo, timeline[i], i)
      Conflator.addTimepoint(deltaSeriesRepo, timeline.endpoint, len(timeline) -1)
    return deltaSeriesRepo
