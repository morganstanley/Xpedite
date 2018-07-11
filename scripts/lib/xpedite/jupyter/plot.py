"""
Visualizer for transaction level timing and pmc statistics

This module helps to generate
  1. Sunburst visualization to provide a hierarchical view of txn timeline
  2. Bipartite visualization to correlate pmc data with sections of code

Author: Dhruv Shekhawat, Morgan Stanley
"""

import json

class NodeNameFactory(object):
  """Factory to build human friendly node names for timeline tree visaulizaton"""

  def __init__(self):
    self.occurenceCount = {}

  def makeName(self, name):
    """Returns a unique name by suffixing occurence count for duplicates"""
    if name in self.occurenceCount:
      count = self.occurenceCount[name]
      self.occurenceCount[name] = count+1
      name = '{}#{}'.format(name, count)
    self.occurenceCount.update({name : 1})
    return name

def buildTxnPlotTree(node, nameFactory=None, plotId=0):
  """Assigns unique plot id to all nodes of the given tree"""
  nameFactory = NodeNameFactory() if not nameFactory else nameFactory
  nodeEntry = {'name' : nameFactory.makeName(node.name)}
  if node.hasChildren():
    children = []
    for child in node.children:
      children.append(buildTxnPlotTree(child, nameFactory, plotId))
      plotId = children[-1]['id'] + 1
    nodeEntry.update({'children' : children, 'id' : plotId})
  else:
    nodeEntry.update({'size' : node.value, 'id' : plotId})
  return nodeEntry

def printTxnPlotTree(txnPlotTree):
  """Displays a hierarchical view of transaction latency"""
  import pprint
  pp = pprint.PrettyPrinter(indent=2)
  pp.pprint(txnPlotTree)

def buildTxnPlot(timeline):
  """Builds a hierarchical visaulizaton of transaction latency"""
  import time
  from xpedite.analytics.timelineTree import buildTimelineTree
  from xpedite.jupyter.templates import loadTxnPlotTreeMarkup
  try:
    timelineTree = buildTimelineTree(timeline)
    txnPlotTree = buildTxnPlotTree(timelineTree)
    jsonData = json.dumps(txnPlotTree)
    sunburstHtml = loadTxnPlotTreeMarkup()
    uid = int(time.time())
    return sunburstHtml.format(uid, jsonData)
  except Exception as ex:
    return str(ex)

def buildPmcJson(timeline, pmcBarScale):
  """
  labels in js are 1,11,111,1111.. and plotLabels are passed to js separately to counter
    lexicographic plotting of x axis
  """
  pmcSum = {pmcName : timeline.endpoint.deltaPmcs[i] for i, pmcName in enumerate(timeline.endpoint.pmcNames)}
  pmuCollection = []
  nameFactory = NodeNameFactory()
  plotLabels = {}
  probeNum = '1'
  for tp in timeline.points[:-1]:
    for i, pmcName in enumerate(tp.pmcNames):
      pmu = [pmcName]
      plotLabels[pmcName] = pmcName
      pmu.append(probeNum)
      plotLabels[probeNum] = nameFactory.makeName(tp.name)
      pmu.append(float(float((tp.deltaPmcs[i]*pmcBarScale))/pmcSum[pmcName]) if pmcSum[pmcName] > 0 else 0)
      pmuCollection.append(pmu)
    probeNum += '1'
  pmuJson = json.dumps(pmuCollection)
  plotLabelsJson = json.dumps(plotLabels)
  pmcSumJson = json.dumps(pmcSum)
  return pmuJson, plotLabelsJson, pmcSumJson


def buildPmcPlot(timeline):
  """Builds a visaulizaton for correlating pmu metrics with sections of code"""
  from xpedite.jupyter.templates import loadTxnPmcMarkup
  from xpedite.util import makeUniqueId
  from xpedite.report.markup import VIZ
  try:
    pmcCount = len(timeline[0].pmcNames) if timeline[0].pmcNames else 0
    if pmcCount <= 0:
      return None
    pmcBarScale = 100 / pmcCount #get scale size
    pmuJson, plotLabelsJson, pmcSumJson = buildPmcJson(timeline, pmcBarScale)
    bipartiteHtml = loadTxnPmcMarkup()
    uid = makeUniqueId()
    bipartiteHtml = bipartiteHtml % (uid, VIZ, pmuJson, plotLabelsJson,
      pmcSumJson, pmcBarScale, pmcCount, uid)
    return bipartiteHtml
  except Exception as ex:
    return str(ex)
