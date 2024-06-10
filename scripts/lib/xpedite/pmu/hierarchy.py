#!/usr/bin/env python
"""
Topdown hierarchy builder

This module is used to build, topdown hierarchy for all supported micro architectures.
The module also supports resolution of pmc events for nodes in the hierarchy.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import types
from collections            import OrderedDict, defaultdict, Counter
from xpedite.pmu.uarchEvent import GenericCoreEvent, FixedCoreEvent, OffCoreEvent
from xpedite.dependencies   import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Termcolor)
from termcolor              import colored  # pylint: disable=wrong-import-position


class Root(object):
  """Root of the topdown hierarchy tree"""

  name = 'Root'
  domain = 'Slots'
  area = 'FE+BE'
  desc = """
  Root of top down micro architecture analysis hierarchy.
"""
  level = 0
  htoff = False
  sample = []
  errcount = 0
  server = True

  def __init__(self):
    self.children = []

  @staticmethod
  def compute(_):
    """Default compute implementation for nodes of topdown hierarchy"""
    raise Exception('compute not avaliable for root node')

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class UnknownEvent(object):
  """Class to model events not known to Xpedite"""

  eventType = 'Unknown'
  def __init__(self, name):
    self.eventType = UnknownEvent.eventType
    self.name = name

  def __repr__(self):
    return 'unknown event - {}'.format(self.name)

class Hierarchy(object):
  """Hierarchy for topdown micro architecture analysis"""

  def __init__(self, eventsDb):
    self.eventsDb = eventsDb
    self.root = Root()
    self.nodes = OrderedDict({Root.name : self.root})
    self.maxLevel = 0
    self.levels = defaultdict(list)
    self.metrics = OrderedDict()

  @staticmethod
  def formatName(name):
    """Formats name by removing _"""
    return name.replace('_', '')

  @staticmethod
  def buildEventsCollector(events):
    """Returns delegate used to gather pmu events for nodes in a topdown hierarchy"""
    def delegate(event, level):
      """
      Appends the given event to an events collection

      :param event: Event to be added
      :param level: Level of the node in the topdown tree

      """
      if isinstance(event, types.LambdaType):
        return event(delegate, level)
      events[event] = None
      return 99
    return delegate

  def run(self, node):
    """
    Callback to gather nodes in the topdown hierarchy

    :param node: Node in a topdown hierarchy tree

    """
    node.thresh = False
    self.maxLevel = max(node.level, self.maxLevel)
    node.children = []
    node.name = self.formatName(node.name)
    self.nodes.update({node.name: node})

  def metric(self, metric):
    """Callback to gather predefined metrics"""
    metric.name = self.formatName(metric.name)
    self.metrics.update({metric.name: metric})

  def buildHierarchy(self):
    """Method to build hierarchy for topdown micro architecture analysis"""
    for node in self.nodes.values():
      if hasattr(node, 'parent'):
        node.parent.children.append(node)
      if node.level == 1:
        node.parent = self.root # pylint: disable=attribute-defined-outside-init
        self.root.children.append(node)
      self.levels[node.level].append(node)

    for node in self.nodes.values():
      node.events = self.events(node) # pylint: disable=attribute-defined-outside-init
      node.computeValue = types.MethodType(Hierarchy.computeTopdownNodeMetrics, node) # pylint: disable=attribute-defined-outside-init
      evtMap = self.aggregateEventsByType(node.events)
      node.genericPmcCount = evtMap[GenericCoreEvent.eventType] if GenericCoreEvent.eventType in evtMap else 0 # pylint: disable=attribute-defined-outside-init
      node.fixedPmcCount = evtMap[FixedCoreEvent.eventType]   if FixedCoreEvent.eventType in evtMap else 0 # pylint: disable=attribute-defined-outside-init
      node.offcorePmcCount = evtMap[OffCoreEvent.eventType] if OffCoreEvent.eventType in evtMap else 0 # pylint: disable=attribute-defined-outside-init
      node.uncorePmcCount = evtMap[UnknownEvent.eventType] if UnknownEvent.eventType in evtMap else 0 # pylint: disable=attribute-defined-outside-init
      node.supported = self.isNodeSupported(node) # pylint: disable=attribute-defined-outside-init

    for metric in self.metrics.values():
      events = OrderedDict()
      metric.compute(self.buildEventsCollector(events))
      metric.events = self.resolveEvents(events)
      metric.computeValue = types.MethodType(Hierarchy.computeTopdownMetric, metric)

  @staticmethod
  def buildEventRetriever(counterMap):
    """Returns delegate to locate value for pmu events in a counter map"""
    def delegate(event, level):
      """
      Returns value of a PMC event from counter map

      :param event: PMC event being looked up
      :param level: Level of node in the topdown hierarchy tree

      """
      if isinstance(event, types.LambdaType):
        return event(delegate, level)
      return counterMap[event]
    return delegate

  @staticmethod
  def computeTopdownNodeMetrics(node, counterMap):
    """
    Computes values of a topdown node

    :param node: Topdown Node to be computed
    :param counterMap: A map to locate pmc values from counter object

    """
    delegate = Hierarchy.buildEventRetriever(counterMap)
    topdownValues = []
    if node.children:
      for child in node.children:
        child.compute(delegate)
        topdownValues.append(TopdownValue(child.name, child.val * 100, child.thresh))
    else:
      node.compute(delegate)
      topdownValues.append(TopdownValue(node.name, node.val * 100, node.thresh))
    return topdownValues

  @staticmethod
  def computeTopdownMetric(metric, counterMap):
    """
    Computes values of a predefined metric

    :param node: Metric to be computed
    :param counterMap: A map to locate pmc values from counter object

    """
    delegate = Hierarchy.buildEventRetriever(counterMap)
    metric.compute(delegate)
    return TopdownValue(metric.name, metric.val, False)

  def resolveEvents(self, eventNames):
    """
    Method to lookup event objects for a given list of event names

    :param eventNames: A list of event names to look up

    """
    return [self.eventsDb[event] if event in self.eventsDb else UnknownEvent(event) for event in eventNames]

  def events(self, node):
    """
    Gathers pmu uarch events for a node in the topdown hierarchy

    :param node: Node to lookup events for

    """
    events = OrderedDict()
    delegate = self.buildEventsCollector(events)
    if node.children:
      for child in node.children:
        child.compute(delegate)
    else:
      node.compute(delegate)
    return self.resolveEvents(events)

  @staticmethod
  def aggregateEventsByType(events):
    """
    Aggregates count of one or more events by type

    :param events: A list of events to be aggregated

    """
    return Counter(event.eventType for event in events)

  @staticmethod
  def isNodeSupported(node):
    """
    Checks if value for a topdown node can be computed

    Nodes that use at most 8 general purpose and 2 offcore registers are supported

    :param node: Node to check support status

    """
    return node.genericPmcCount <= 8 and node.offcorePmcCount <= 2 and node.uncorePmcCount <= 0

  def node(self, nodeName):
    """
    Returns topdown node for the given name

    :param nodeName: Name of the node to lookup

    """
    return self.nodes[nodeName] if nodeName in self.nodes else None

  @staticmethod
  def isGenericPmcSupported(pmcCount):
    """
    Checks support for programming the given number of general purpose registers

    :param pmcCount: Number of general purpose registers to be programmed

    """
    return pmcCount <= 8

  @staticmethod
  def isFixedPmcSupported(pmcCount):
    """
    Checks support for programming the given number of fixed purpose registers

    :param pmcCount: Number of fixed purpose registers to be programmed

    """
    return pmcCount <= 3

  @staticmethod
  def isOffcorePmcSupported(pmcCount):
    """
    Checks support for programming the given number of offcore registers

    :param pmcCount: Number of offcore registers to be programmed

    """
    return pmcCount <= 2

  @staticmethod
  def isUncorePmcSupported(pmcCount):
    """
    Checks support for programming the given number of uncore registers

    :param pmcCount: Number of uncore registers to be programmed

    """
    return pmcCount <= 0

  def _toString(self, nodes):
    """Formats topdown hierarchy to string"""
    hierarchyStr = ''
    for node in nodes:
      good = 'green'
      bad = 'red'
      neutral = 'yellow'
      color = good if node.supported else bad
      supported = node.supported
      eventStr = '{} - {} | {} - {} | {} - {} | {} {} | {} {}'.format(
        colored(
          'events', neutral if supported else bad
        ),
        colored(
          '{:2d}'.format(len(node.events)), good if supported else bad
        ),
        colored('generic', neutral if self.isGenericPmcSupported(node.genericPmcCount) else bad),
        colored(
          '{:2d}'.format(node.genericPmcCount), good if self.isGenericPmcSupported(node.genericPmcCount) else bad
        ),
        colored(
          'fixed', neutral if self.isFixedPmcSupported(node.fixedPmcCount) else bad
        ),
        colored(
          '{:2d}'.format(node.fixedPmcCount), good if self.isFixedPmcSupported(node.fixedPmcCount) else bad
        ),
        colored(
          'offcore', neutral if self.isOffcorePmcSupported(node.offcorePmcCount) else bad
        ),
        colored(
          '{:2d}'.format(node.offcorePmcCount), good if self.isOffcorePmcSupported(node.offcorePmcCount) else bad
        ),
        colored(
          'unknown', neutral if self.isOffcorePmcSupported(node.uncorePmcCount) else bad
        ),
        colored(
          '{:2d}'.format(node.uncorePmcCount), good if self.isOffcorePmcSupported(node.uncorePmcCount) else bad
        ),
      )
      indent = '\n{} |--[Lvl-{}]'.format('    ' * (node.level), node.level)
      nodeStr = '{} {}'.format(indent, colored(node.name, color))
      hierarchyStr += '{:60s}(children - {} | {})'.format(nodeStr, len(node.children), eventStr)
      hierarchyStr += self._toString(node.children)
    return hierarchyStr

  def __repr__(self):
    """Returns a string representation of this hierarchy"""
    hierarchyStr = 'metrics [{}]\n'.format(', '.join(self.metrics.keys()))
    hierarchyTreeStr = self._toString([self.root])
    line = '\n{}'.format(('=' * 90))
    hierarchyStr += '{0}\nTop down pmu hierarchy{0}{1}\n{0}'.format(line, hierarchyTreeStr)
    return hierarchyStr

class TopdownValue(object):
  """Stores computed value for a topdown node or a predefined metric"""

  def __init__(self, name, value, breached):
    self.name = name
    self.value = value
    self.breached = breached

  def __repr__(self):
    color = 'red' if self.breached else 'green'
    return '{} - {}'.format(colored(self.name, color), self.value)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__
