#!/usr/bin/env python
"""
Class definitions for using performance counters and topdown metrics

This module defines the following classes
  1. Event - a supported micro architectural event
  2. TopdownNode - a node in the topdown hierarchy
  3. Metric - commonly used cpu metrics like IPC, CPI etc...
  4. TopdownMetrics - container for topdown nodes and metrics
  5. EventSet - container for generic, fixed and offcore requests

Author: Manikandan Dhamodharan, Morgan Stanley
"""

class Event(object):
  """Stores the name and programmable attributes of a pmu event"""

  def __init__(self, name, uarchName, user=True, kernel=True):
    self.name = name
    self.uarchName = uarchName
    self.user = user
    self.kernel = kernel

  def __hash__(self):
    return hash(self.uarchName)

  def __eq__(self, other):
    return self.uarchName == other.uarchName

  def __repr__(self):
    """Returns string representation of a pmu event"""
    flags = None
    if self.user and self.kernel:
      flags = 'user|kernel'
    else:
      flags = 'user' if self.user else 'kernel'
    return '{} [{}]'.format(self.name, flags)

class TopdownNode(object):
  """Stores the name and children of a node in the topdown hierarchy"""

  def __init__(self, name, children=None):
    self.name = name
    self.children = children

  def visit(self, metrics):
    """Gathers metrics from self or children of a node in the topdown hierarchy"""
    if self.children:
      for child in self.children:
        metrics.append(child)
    else:
      metrics.append(self.name)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class Metric(object):
  """Stores the name of a pre defined metric for a cpu micro architecture"""

  def __init__(self, name):
    self.name = name

  def visit(self, metrics):
    """Gathers name of predefined metrics"""
    metrics.append(self.name)

  def __repr__(self):
    return str(self.__dict__)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class TopdownMetrics(object):
  """Container for storing one or more topdown nodes and micro architectural metrices"""

  def __init__(self):
    self.nodes = []
    self.metrics = []

  def add(self, topdown, obj, condition=lambda n: True):
    """Adds a node or metric to this collection"""
    if isinstance(obj, TopdownNode):
      node = topdown.node(obj.name)
      if condition(node):
        self.nodes.append(node)
        return node
    elif isinstance(obj, Metric):
      metric = topdown.metric(obj.name)
      if condition(metric):
        self.metrics.append(metric)
        return metric
    return None

  def nodeKeys(self):
    """Returns a list of topdown nodes in this collection"""
    keys = []
    for node in self.nodes:
      children = [child.name for child in node.children] if node.children else None
      keys.append(TopdownNode(node.name, children))
    return keys

  def metricKeys(self):
    """Returns a list of metric names in this collection"""
    return [Metric(metric.name) for metric in self.metrics]

  def topdownKeys(self):
    """Returns the names of both topdown nodes and metrices in this collection"""
    return self.nodeKeys() + self.metricKeys()

  def names(self):
    """Returns names of both topdown nodes and metrices in this collection"""
    names = []
    for node in self.nodes:
      if node.children:
        for child in node.children:
          names.append(child.name)
      else:
        names.append(node.name)
    for metric in self.metrics:
      names.append(metric.name)
    return names

  def compute(self, counterMap):
    """Computes values of topdown nodes and metrices in this collection"""
    topdownValues = []
    for node in self.nodes:
      topdownValues += node.computeValue(counterMap)
    for metric in self.metrics:
      topdownValues.append(metric.computeValue(counterMap))
    return topdownValues

  def __eq__(self, other):
    if other:
      return (
        {node.name for node in self.nodes} == {node.name for node in other.nodes} and
        {metric.name for metric in self.metrics} == {metric.name for metric in other.metrics}
      )
    return False

  def __repr__(self):
    return str(self.__dict__)

class EventSet(object):
  """Container for requests (generic, offcore and fixed) to program pmu events"""

  def __init__(self, cpuSet):
    self.cpuSet = cpuSet
    self.genericRequests = []
    self.offcoreRequests = []
    self.fixedRequests = []

  def addFixedPmuRequest(self, request):
    """
    Adds a request to program fixed pmu registers to this collection

    :param request: fixed pmu request to be added

    """
    if len(self.fixedRequests) >= 3:
      raise Exception('failed to add fixed pmu request to group - maximum supported event count (3) reached')
    self.fixedRequests.append(request)
    self.fixedRequests = sorted(self.fixedRequests, key=lambda e: e.ctrIndex)

  def addGenericPmuRequest(self, request):
    """
    Adds a request to program generic pmu registers to this collection

    :param request: generic pmu request to be added

    """
    if len(self.genericRequests) >= 8:
      raise Exception('failed to add generic pmu request to group - maximum supported event count (8) reached')
    self.genericRequests.append(request)

  def addOffcorePmuRequest(self, request):
    """
    Adds a request to program offcore pmu registers to this collection

    :param request: offcore pmu request to be added

    """
    if len(self.offcoreRequests) >= 2:
      raise Exception('failed to add offcore pmu request to group - maximum supported event count (2) reached')
    self.offcoreRequests.append(request)

  def requests(self):
    """Returns the list of generic and fixed pmu requests in this collection"""
    return self.genericRequests + self.fixedRequests

  def genericRequestCount(self):
    """Returns the list of generic pmu requests in this collection"""
    return len(self.genericRequests)

  def offcoreRequestCount(self):
    """Returns the list of offcore pmu requests in this collection"""
    return len(self.offcoreRequests)

  def fixedRequestCount(self):
    """Returns the list of fixed pmu requests in this collection"""
    return len(self.fixedRequests)

  def __len__(self):
    return len(self.genericRequests) + len(self.fixedRequests)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

  def __repr__(self):
    reprStr = 'cpu set - {}'.format(self.cpuSet)
    if self.genericRequests:
      reprStr += '\n'
      for event in self.genericRequests:
        reprStr += '\t Core event {}'.format(event)

    if self.fixedRequests:
      reprStr += '\n'
      for event in self.fixedRequests:
        reprStr += '\t Core event {}'.format(event)

    if self.offcoreRequests:
      reprStr += '\n'
      for event in self.offcoreRequests:
        reprStr += '\t OffCore event {}'.format(event)
    return reprStr
