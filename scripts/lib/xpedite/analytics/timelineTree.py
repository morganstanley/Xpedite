"""
Module to build a hierarchical break up of time spend in a transaction.

This module accepts a timeline with a sequence of timepoints (between each pair of probes)
and aggregates them to build a hierarchical structure suitable for drill down.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.dependencies import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Termcolor)
from termcolor import colored # pylint: disable=wrong-import-position

class Node(object):
  """A node in a N-ary tree hierarchy"""

  def __init__(self, name):
    self.name = name
    self.children = []

  def addChild(self, node):
    """
    Adds a child node to this node

    :param node: Child node to add

    """
    self.children.append(node)

  @staticmethod
  def hasChildren():
    """Checks if this node has any children"""
    return True

  def __len__(self):
    return len(self.children)

  @staticmethod
  def _toString(nodes, level):
    """
    Returns string representation for a list of nodes

    :param nodes: List of node to be formatted
    :param level: Depth of node from tree root

    """
    nodeStr = ''
    for node in nodes:
      indent = '\n{} |--[Lvl-{}]'.format('    ' * level, level)
      nodeStr = '{} {}'.format(indent, colored(node.name, 'yellow'))
      if hasattr(node, 'children'):
        nodeStr += '{:60s}(children - {})'.format(nodeStr, len(node.children))
        nodeStr += Node._toString(node.children, level +1)
      else:
        nodeStr += '{:60s}(value - {})'.format(nodeStr, node.value)
    return nodeStr

  def __repr__(self):
    return Node._toString([self], 1)

class Leaf(object):
  """A leaf Node in a N-ary tree hierarchy"""

  def __init__(self, name, value):
    self.name = name
    self.value = value

  @staticmethod
  def hasChildren():
    """Always returns False, as leaf nodes cannot have children"""
    return False

def buildTimelineTree(timeline, threshold=None, begin=None, end=None, childCount=4, node=None):
  """
  Builds a hierarchical view of a given timeline

  :param timeline: Timeline be transformed
  :param threshold: Threshold for skipping insignificant nodes (Default value = None)
  :param begin: Begin index for the range of nodes at a level (Default value = None)
  :param end: End index for the range of nodes at a level (Default value = None)
  :param childCount: Number of children at a level (Default value = 4)
  :param node: Parent for the nodes at the current level (Default value = None)

  """
  threshold = timeline.endpoint.duration / childCount if not threshold else threshold
  begin = 0 if not begin else begin
  end = len(timeline) -1 if not end else end
  node = Node('root') if node is None else node
  duration = 0
  groupBegin = begin
  for i in range(begin, end):
    timepoint = timeline[i]
    duration += timepoint.duration
    canGroup = (end - i > childCount - len(node))
    if (duration >= threshold) or not canGroup:
      child = None
      if canGroup:
        child = Node('{} - {}'.format(
          timeline[groupBegin].name, timeline[i+1].name
        ))
        buildTimelineTree(timeline, duration/childCount, groupBegin, i+1, childCount, child)
      else:
        name = '{} - {}'.format(timepoint.name, timeline[i+1].name)
        child = Leaf(name, timepoint.duration)
      node.addChild(child)
      groupBegin = i + 1
      duration = 0
  return node
