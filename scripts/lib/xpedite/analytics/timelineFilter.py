"""
Module to filter timelines based on arbitrary criteria

This module accepts a list of source profiles and builds new ones
by filtering the timeline in source based on a predicate.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.profiler.profile import Profiles, Profile

class TimelineFilter(object):
  """Implements logic to select a subset of timelines matching a filter criteria"""

  def __init__(self, predicate):
    self.predicate = predicate

  def filterTimelines(self, timelineCollection):
    """
    Filters timelines from a collection

    :param timelineCollection: Timeline collection to be filtered

    """
    filteredTlc = []
    for timeline in timelineCollection:
      if self.predicate(timeline):
        filteredTlc.append(timeline)
    return filteredTlc

  def filterTimelineStats(self, tls):
    """
    Filters timelines from a timeline statistics object

    :param tls: timeline statistics object to be filtered

    """
    from xpedite.analytics.conflator import Conflator
    filteredTls = Conflator.createTimelineStats(tls, tls.category, tls.route)
    filteredTls.timelineCollection = self.filterTimelines(tls.timelineCollection)
    Conflator.buildDeltaSeriesRepo(filteredTls)
    return filteredTls

  def filterProfile(self, srcProfile):
    """
    Filters timelines from a source profile

    :param srcProfile: source profile to be filtered

    """
    current = self.filterTimelineStats(srcProfile.current)
    if not current:
      return None
    dstProfile = Profile(srcProfile.name, current, {})
    for name, tls in srcProfile.benchmarks.items():
      filteredtls = self.filterTimelineStats(tls)
      if filteredtls:
        dstProfile.benchmarks.update({name:filteredtls})
    return dstProfile

  def apply(self, profiles):
    """
    Filters timelines from a collection of profiles

    :param profiles: collection of profiles to be filtered

    """
    filtredProfiles = Profiles(profiles.name, profiles.transactionRepo)
    for profile in profiles:
      filteredProfile = self.filterProfile(profile)
      if filteredProfile:
        filtredProfiles.addProfile(filteredProfile)
    return filtredProfiles

def locateTimeline(profiles, txnId):
  """
  Performs a lookup of timeline object with txnId

  :param profiles: profiles holding a collection of timelines
  :param txnId: txnId of the timeline to lookup

  """
  for profile in profiles:
    for timeline in profile.current.timelineCollection:
      if timeline.txnId == txnId:
        return timeline
  return None
