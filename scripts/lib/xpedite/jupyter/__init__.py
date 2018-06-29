"""
This package provides modules and drivers to support real time analytics
on Xpedite profile data

This modules include
  1. Driver to start a jupyter instance
  2. Modules to collect and serialize profiles and html reports
  3. Logic to generate and provide code snippets
  4. Logic to integrate Xpedite commands and visualizations in jupyter shell

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.dependencies import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(
  Package.Enum,
  Package.Futures,
  Package.NbFormat,
  Package.JsonSchema,
  Package.FuncTools,
  Package.Traitlets,
  Package.Six,
  Package.Notebook,
  Package.IPythonGenUtils
)

DATA_DIR = 'xpData'
DATA_FILE_EXT = '.xpd'
NOTEBOOK_EXT = '.ipynb'
ARCHIVE_FILE_EXT = '.tar{}'.format(DATA_FILE_EXT)
EXPORT_PREFIX = 'xpediteExport'
SHELL_PREFIX = 'xpediteShell'
TEMP_PREFIX = 'xpedite'


def buildXpdName(notebookName):
  """
  Builds a name for xpedite data file, by substituting the file extension

  :param notebookName: Name of the jupyter notebook

  """
  dataFile = notebookName.replace(NOTEBOOK_EXT, DATA_FILE_EXT)
  return dataFile
