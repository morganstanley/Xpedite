#!/usr/bin/env python
"""
Module to load and cache topdown hierarchy for supported micro architectures

Usage: This script displays the topdown hierarchy and supported metrics:
To run : pushd ../..; python -m xpedite.pmu.topdown <options>; popd

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import logging
import argparse
from xpedite.pmu.hierarchy   import Hierarchy
from xpedite.pmu.eventsDb    import EventsDbCache

LOGGER = logging.getLogger(__name__)

class NodeFormatter(object):
  """Formats nodes of a topdown hierarchy"""

  @staticmethod
  def eventsToString(events):
    """Formats a list of events to string"""
    eventsStr = ''
    for event in events:
      eventsStr += '\n{}'.format(event)
    return eventsStr

  @staticmethod
  def toString(nodes):
    """Formats a list of nodes to string"""
    delimiter = '-'*60
    eventsStr = ''
    for node in nodes:
      header = '\n{0} events for node {1} {0}'.format(delimiter, node.name)
      headerLen = len(header)
      eventsStr += header
      eventsStr += NodeFormatter.eventsToString(node.events)
      eventsStr += '\n{}'.format('-' * headerLen)
    return eventsStr

class Topdown(object):
  """Topdown hierarchy for a cpu micro architecture"""

  def __init__(self, eventsDb):
    self.hierarchy = Hierarchy(eventsDb)
    ratios = eventsDb.topdownRatios()
    ratios.Setup(self.hierarchy)
    self.hierarchy.buildHierarchy()

  def node(self, name):
    """
    Lookup topdown node with the given name

    :param name: Name of node to lookup

    """
    node = self.hierarchy.node(name)
    if not node:
      raise Exception('failed to locate node {} in top down hierarchy'.format(name))
    return node

  def metric(self, name):
    """
    Lookup predefined metric with the given name

    :param name: Name of metric to lookup

    """
    if name in self.hierarchy.metrics:
      return self.hierarchy.metrics[name]
    raise Exception('failed to locate node {} in top down hierarchy'.format(name))

  def metrics(self):
    """Returns a list of predefined metrics"""
    return self.hierarchy.metrics

  @staticmethod
  def filterNode(nodes, name):
    """
    Filters nodes by name

    :param nodes: A list of nodes to be filtered
    :param name:  Name to be used for filtering

    """
    if name.lower() == 'all':
      return nodes.values()
    if name in nodes:
      return [nodes[name]]
    return None

  def nodesToString(self, name):
    """Filters nodes by name and returns a formatted string"""
    nodes = self.filterNode(self.hierarchy.nodes, name)
    if nodes:
      return NodeFormatter.toString(nodes)
    return None

  def metricsToString(self, name):
    """Filters metrics by name and returns a formatted string"""
    metrics = self.filterNode(self.hierarchy.metrics, name)
    return NodeFormatter.toString(metrics)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class TopdownCache(object):
  """A cache of topdown hierrachy for all known micro architectures"""

  def __init__(self, eventsDbCache):
    self.cache = {}
    self.eventsDbCache = eventsDbCache

  def get(self, cpuId):
    """
    Lookup topdown hierarchy for a cpu micro architecture

    :param cpuId: Id of the cpu model to lookup

    """
    if cpuId in self.cache:
      return self.cache[cpuId]
    eventsDb = self.eventsDbCache.get(cpuId)
    topdown = Topdown(eventsDb)
    self.cache.update({cpuId : topdown})
    return topdown

def main():
  """Displays topdown hierarchy for localhost"""
  parser = argparse.ArgumentParser(description='Top down micro architecture analysis')
  parser.add_argument('--node', type=str, help='list pmu events for node - enter "all" to list events for all nodes')
  args = parser.parse_args()
  topdownCache = TopdownCache(EventsDbCache())
  from xpedite.util import getCpuId
  topdownHandle = topdownCache.get(getCpuId())

  if args.node:
    LOGGER.info('%s', topdownHandle.nodesToString(args.node))
    return

  LOGGER.info('%s', topdownHandle.hierarchy)

if __name__ == '__main__':
  main()
