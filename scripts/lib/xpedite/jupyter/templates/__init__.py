"""
Code and markup templates for init cell and plot visualizations

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import logging

LOGGER = logging.getLogger(__name__)

def loadTemplate(name, description):
  """
  Returns contents of a template file

  :param name: Name of the template file
  :param description: description of the template file

  """
  path = os.path.join(os.path.dirname(__file__), '{}'.format(name))
  try:
    with open(path, 'r') as fileHandle:
      return fileHandle.read()
  except IOError as ex:
    LOGGER.exception('Could not load %s from file %s', description, path)
    raise ex

def loadEcgWidget():
  """Returns cell markup for each of the txn categories"""
  return loadTemplate('ecg.fmt', 'code for instantiating the widget')

def loadCategoryMarkup():
  """Returns cell markup for each of the txn categories"""
  return loadTemplate('category.fmt', 'markup for txn categories')

def loadInitCell():
  """Returns python source for init cell"""
  return loadTemplate('initCell.fmt', 'python source for init cell')

def loadTxnPlotTreeMarkup():
  """Returns markup for visualizing txn hierarchical latency breakup"""
  return loadTemplate('sunburst.html', 'markup for txn hierarchical breakup visaulization')

def loadTxnPmcMarkup():
  """Returns markup for visaulizing correlation of txn pmu with sections of code"""
  return loadTemplate('bipartite.html', 'markup for txn pmu visaulization')

def loadLiveInitCell():
  """Returns python source for realtime mode init cell"""
  return loadTemplate('initCellLive.fmt', 'python source for init cell')

def loadReportCell():
  """Returns python source for realtime mode report cell"""
  code = loadTemplate('reportCell.fmt', 'python source for report cell')
  return code

def loadProfilingCell():
  """Returns python source for realtime mode profiling execution cell"""
  code = loadTemplate('profilingCell.fmt', 'python source for tmp init cell')
  return code
