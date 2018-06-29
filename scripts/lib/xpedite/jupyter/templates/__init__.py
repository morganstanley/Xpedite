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
