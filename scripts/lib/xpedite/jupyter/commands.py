"""
Xpedite shell commands to query, filter and visualize transactions and profile data

This module implements the following commands, for use in jupyter shell
  1. txns - returns all transactions, that match a given route
  2. plot - plots a collection of transactions, that match a route
          - given an integer argument, plot render visualization of
            the transaction with id matching the argument
  3. stat - Generates statistics for a collection of transaction with the given route
  4. filter - filters transactions matching the given criteria
  5. diff - Compares statistics for a pair or a group of transactions

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import logging
from IPython.display           import display, HTML
from xpedite.jupyter.context   import context
from xpedite.report.markup     import ERROR_TEXT

LOGGER = logging.getLogger(__name__)

def globalProfile():
  """Returns global profile object for the current profile session"""
  return context.profiles

def conflate(profiles, routePoints):
  """
  Conflates timelines across multiple profiles to one

  :param profiles: profiles to be conflated
  :param routePoints: target route for conflation

  """
  # no need to check if profiles is None, as globalProfiles() will raise exception for invalid profiles
  profiles = profiles if profiles else globalProfile()
  if not routePoints:
    route = profiles[0].route
  elif len(routePoints) == 1:
    LOGGER.error('Not enough route points.')
    return None
  else:
    from xpedite.types.probe import Probe
    from xpedite.types.route import Route
    probes = [Probe(probeName, probeName) for probeName in routePoints]
    route = Route(probes)
  profile = context.conflator.conflateProfiles(profiles, route, '')
  if not profile.current.timelineCollection:
    LOGGER.error('Route %s not found.', route)
    return None
  return profile

def routes():
  """Returns the list of routes found in the current profile session"""
  routeList = []
  for profile in globalProfile():
    routeList.append(profile.route)
  return routeList

def plot(routePoints=None, profiles=None):
  """
  Creates visualization for a one or more conflated timeline(s)

  :param routePoints: Indices of the conflated route (Default value = None)
  :param profiles: Profiles with timelines to plot (Default value = None)

  """
  if isinstance(routePoints, int):
    from xpedite.jupyter.plot               import buildTxnPlot, buildPmcPlot
    from xpedite.analytics.timelineFilter   import locateTimeline
    profiles = profiles if profiles else globalProfile()
    txnId = routePoints
    timeline = locateTimeline(profiles, txnId)
    if timeline:
      display(HTML(buildTxnPlot(timeline)))
      if profiles.pmcNames:
        display(HTML(buildPmcPlot(timeline)))
      return
    display(HTML(ERROR_TEXT.format('cannot find transaction for id {}'.format(txnId))))
    return
  from xpedite.report.flot import FlotBuilder
  profile = conflate(profiles, routePoints)
  if profile:
    flot = FlotBuilder().buildBenchmarkFlot(profile.category, profile.current, profile.benchmarks)
    display(HTML(str(flot)))

def stat(routePoints=None, profiles=None):
  """
  Creates statistics for a collection of conflated timelines

  :param routePoints: Indices of the conflated route (Default value = None)
  :param profiles: Profiles with data for building stats (Default value = None)

  """
  from xpedite.report.stats import StatsBuilder
  profile = conflate(profiles, routePoints)
  if profile:
    stats = StatsBuilder().buildStatsTable(profile.category, profile.current, profile.benchmarks)
    display(HTML(str(stats)))

def diffTxn(lhs, rhs, profiles):
  """
  Compares duration/pmc values for a pair of transactions

  :param lhs: Transaction id (lhs value of comparison)
  :type lhs: int
  :param rhs: Transaction id (rhs value of comparison)
  :type rhs: int
  :param profiles: Transactions from the current profile session
  :type profiles: xpedite.report.profile.Profiles

  """
  from xpedite.analytics.timelineFilter   import locateTimeline
  from xpedite.report.diff                import DiffBuilder
  from xpedite.types.route                import conflateRoutes
  from xpedite.analytics.conflator        import Conflator

  timeline1 = locateTimeline(profiles, lhs)
  timeline2 = locateTimeline(profiles, rhs)
  if timeline1 and timeline2:
    lhs, rhs = (timeline1, timeline2) if len(timeline1) > len(timeline2) else (timeline2, timeline1)
    routeIndices = conflateRoutes(lhs.txn.route, rhs.txn.route)
    if not routeIndices:
      display(HTML(ERROR_TEXT.format('Transactions {} and {} are not comparable'.format(lhs.txnId, rhs.txnId))))
      return
    topdownMetrics = Conflator().getTopdownMetrics(profiles.cpuInfo.cpuId, profiles.topdownKeys)
    conflatedTimeline = Conflator().conflateTimeline(lhs, routeIndices, profiles.eventsMap, topdownMetrics)
    display(HTML(str(DiffBuilder().buildDiffTable(conflatedTimeline, rhs))))
  else:
    if not (timeline1 or timeline2):
      display(HTML(ERROR_TEXT.format(
        'Can\'t find transactions. Are these ({} and {}) valid txn id\'s?'.format(lhs, rhs)
      )))
    else:
      txnId = rhs if timeline1 else lhs
      display(HTML(ERROR_TEXT.format('Can\'t find transaction {}, is the txn id valid?'.format(txnId))))

def diffTxns(lhs, rhs, profiles):
  """
  Compares statistics for a group of transactions

  :param lhs: A list of transaction ids (lhs value of comparison)
  :param rhs: A list of transaction ids (rhs value of comparison)
  :param profiles: Transactions from the current profile session
  :type profiles: xpedite.report.profile.Profiles

  """
  from xpedite.analytics.timelineFilter   import locateTimeline
  from xpedite.analytics.conflator        import Conflator
  from xpedite.report.stats               import StatsBuilder

  lhs = [lhs] if not isinstance(lhs, list) else lhs
  rhs = [rhs] if not isinstance(rhs, list) else rhs
  if not all(isinstance(txn, int) for txn in lhs) or not all(isinstance(txn, int) for txn in rhs):
    display(HTML(ERROR_TEXT.format(
      'Arguments must contain only valid txn ids (integers), example - diff([1, 2, 3, ... ], 1000)'
    )))
    return
  try:
    _ = [locateTimeline(profiles, timeline).txn for timeline in lhs]
    _ = [locateTimeline(profiles, timeline).txn for timeline in rhs]
  except Exception:
    display(HTML(ERROR_TEXT.format(
      'Can\'t locate one or more transactions, Are these valid txn id\'s?'
    )))
    return

  lhsProfiles = filter(lambda txn: txn.txnId in lhs)
  rhsProfiles = filter(lambda txn: txn.txnId in rhs)

  for i, profile in enumerate(lhsProfiles.profiles):
    category = 'Route #{}'.format(i)
    rhsProfile = Conflator().conflateProfiles(rhsProfiles.profiles, profile.route, category)
    if rhsProfile and len(rhsProfile.current) == 1 and len(profile.current) == 1:
      diffTxn(profile.current[0].txnId, rhsProfile.current[0].txnId, profiles)
    elif rhsProfile and rhsProfile.current:
      display(HTML(str(StatsBuilder().buildStatsTable(category, profile.current, {category : rhsProfile.current}))))
    else:
      display(HTML(ERROR_TEXT.format(
        'route {} with {} txns occurs only in one side of equality'.format(profile.route, rhs.current)
      )))

def diff(lhs, rhs, profiles=None):
  """
  Compares statistics for a pair or a group of transactions

  :param lhs: Transaction ID or A list of transaction ids (lhs value of comparison)
  :param rhs: Transaction ID or A list of transaction ids (rhs value of comparison)
  :param profiles: Transactions from the current profile session
  :type profiles: xpedite.report.profile.Profiles

  """
  profiles = profiles if profiles else globalProfile()
  if isinstance(lhs, int) and isinstance(rhs, int):
    diffTxn(lhs, rhs, profiles)
  elif isinstance(lhs, list) or isinstance(rhs, list):
    diffTxns(lhs, rhs, profiles)
  else:
    display(HTML(ERROR_TEXT.format(
"""
Invalid arguments:<br>
diff expects either a pair of txns or a pair of list of txns<br>
usage 1: diff(&lt;txnId1&gt;, &lt;txnId2&gt;) - compares two transactions with id txnId1 vs txnId2<br>
usage 2: diff(&lt;List of txns&gt;, &lt;List of txns&gt;) - compares stats for the first list of txns vs the second.<br>
"""
    )))

class Txns(object):
  """Class to hold conflated timelines"""

  def __init__(self, profile):
    self.profile = profile

  def __len__(self):
    return len(self.profile.current.timelineCollection)

  def __getitem__(self, i):
    return self.profile.current.timelineCollection[i]

  def toDataFrameDict(self):
    """Returns transactions in pandas DataFrame dict format"""
    dfDict = {}
    for tl in  self.profile.current.timelineCollection:
      for i in range(len(tl)-1):
        tp = tl[i]
        col = dfDict.get(tp.name)
        if not col:
          col = []
          dfDict.update({tp.name : col})
        col.append(tp.duration)
    return dfDict

  def __repr__(self):
    from xpedite.util import makeUniqueId
    from xpedite.report.reportbuilder   import ReportBuilder
    from xpedite.types                  import ResultOrder
    if self.profile:
      uid = makeUniqueId()
      strRepr = ReportBuilder().buildPmuScript(
        self.profile.current.timelineCollection, uid
      ) if self.profile.current.isEventsEnabled() else ''
      threshold = 1000
      if len(self.profile.current) > threshold:
        LOGGER.warning('too many transaction - showing only %d out of %d', threshold, len(self.profile.current))
      strRepr += str(ReportBuilder().buildTimelineTable(
        self.profile.current, self.profile.probes, ResultOrder.Chronological, threshold, uid
      ))
      display(HTML(strRepr))
    return ''

def txns(routePoints=None, profiles=None):
  """
  Conflates timelines across profiles with the given route points

  :param routePoints: Indices of the conflated route (Default value = None)
  :param profiles: Profiles with data for building stats (Default value = None)

  """
  profile = conflate(profiles, routePoints)
  return Txns(profile)

class FilteredProfiles(object):
  """Class to hold filtered profiles"""

  def __init__(self, profiles):
    self.profiles = profiles

  def __repr__(self):
    return str([Txns(profile) for profile in self.profiles])

  def plot(self, routePoints=None):
    """
    Creates visualization for a one or more conflated timeline(s)

    :param routePoints: Indices of the conflated route (Default value = None)
    """
    return plot(routePoints, self.profiles)

  def stat(self, routePoints=None):
    """
    Creates statistics for a collection of conflated timelines

    :param routePoints: Indices of the conflated route (Default value = None)
    """
    return stat(routePoints, self.profiles)

  def txns(self, routePoints=None):
    """
    Conflates timelines across profiles with the given route points

    :param routePoints: Indices of the conflated route (Default value = None)

    """
    if self.profiles:
      return txns(routePoints, self.profiles)
    display(HTML(ERROR_TEXT.format('No matching transactions found')))
    return None

def filter(predicate): #pylint: disable=redefined-builtin
  """
  Filters timelines across profiles using the given predicate

  :param predicate: predicate to be filter by

  """
  from xpedite.analytics.timelineFilter import TimelineFilter
  profiles = FilteredProfiles(TimelineFilter(predicate).apply(globalProfile()))
  return profiles
