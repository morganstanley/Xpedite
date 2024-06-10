"""
Driver to integrate Xpedite with Jupyter shell.

This module provides
  1. logic to store xpedite results in a jupyter notebook
  2. ability to launch a jupyter instance with xpedite specific configuration
  3. code to generate histogram and profile summary cells

Author: Dhruv Shekhawat, Morgan Stanley
"""

import os
import time
import copy
import logging
import tempfile
import nbformat
from enum                         import Enum
from nbformat                     import v4 as nbf
from xpedite.util                 import formatHumanReadable
from xpedite.util                 import compressText
from xpedite.types                import InvariantViloation
from xpedite.jupyter.context      import Context
from xpedite.jupyter              import PROFILES_KEY

LOGGER = logging.getLogger(__name__)

class Action(Enum):
  """Enumeration for jupyte shell actions"""
  def __str__(self):
    return str(self.value)

  Init = 'init'
  Load = 'load'

class D3Flot(object):
  """Holds data to build d3 historgram visualization"""
  def __init__(self):
    self.xAxisValues = []
    self.xAxisLabels = []
    self.xyCoords = []
    self.legends = []

  def toDict(self):
    """Returns a dict representation of this object"""
    d3FlotDict = {'xAxisValues': self.xAxisValues, 'xAxisLabels': self.xAxisLabels,
      'xyCoords' : self.xyCoords, 'legends' : self.legends
    }
    return d3FlotDict

def buildD3Flot(cell):
  """
  Method to store data for flot generation. In flot.js, xAxisValues are used
   to locate where exactly xAxisLabels need to be placed. xyCoords is a list of
   (x,y) dicts created by interleaving the (x,y) data of every run in order.
   This helps sort the x axis values and compare the bars of different run's at
   close proximity(in order, side by side). As we are guaranteed to have runs
   placed in order, their coloring can be accomplished using formula:
   barColor = colorList[barNum % num of runs].

   Returns a d3Flot object.

  """
  d3Flot = D3Flot()
  for tick in cell.flot.options['xaxis']['ticks']:
    d3Flot.xAxisValues.append(tick[0])
    d3Flot.xAxisLabels.append(tick[1])
  for coord in range(0, len(cell.flot.data[0]['data'])):
    for run, _ in enumerate(cell.flot.data):
      xyCoord = {}
      xyCoord['x'] = cell.flot.data[run]['data'][coord][0]
      xyCoord['y'] = cell.flot.data[run]['data'][coord][1]
      d3Flot.xyCoords.append(xyCoord)
  for legend in cell.flot.data:
    d3Flot.legends.append(legend['label'])
  return d3Flot

def buildReportLink(reportKey, action):
  """Returns an url to uniquely identify a report"""
  return '/xpedite?{}={{0}}&reportKey={}&action={}'.format(Context.notebookPathKey, reportKey, action)

def buildReportCells(nb, result, dataFilePath):
  """
  Method to build the report cells. Populates the
   metadata to be stored in init cell and preloads
   source code for creating flots and html links
   Returns the total num of categories in a run.

  """
  from xpedite.jupyter.snippetsBuilder   import buildSnippets
  from xpedite.jupyter.xpediteData       import XpediteDataFactory
  from xpedite.jupyter.templates         import loadCategoryMarkup

  nb['cells'] = []
  d3Flots = []
  flotCode = loadCategoryMarkup()
  reportCount = 0

  xpdf = XpediteDataFactory(dataFilePath)
  xpdf.appendRecord('envReport', 'environment report', result.envReport.zContent)
  xpdProfiles = copy.deepcopy(result.profiles)
  xpdProfiles.transactionRepo = None
  xpdf.appendRecord(PROFILES_KEY, 'xpedite profiles', xpdProfiles)

  # create and compress snippets
  snippetData = buildSnippets(xpdProfiles)
  zSnippetData = compressText(snippetData)
  xpdf.appendRecord('snippets', 'snippets', zSnippetData)

  cellNum = None
  for cellNum, cell in enumerate(result.reportCells):
    linksCode = ''
    d3Flot = buildD3Flot(cell)

    # populate create html links for reports
    reportNum = None
    for reportNum, report in enumerate(cell.htmlReport):
      reportCount += 1
      xpdKey = 'report{}'.format(reportCount)
      linksCode += '<li><a href={} target="_blank">{}</a></li>'.format(
        buildReportLink(xpdKey, Action.Load), report.description
      )
      xpdf.appendRecord(xpdKey, 'htmlReport', report.zContent)

    # populate init cell metadata
    d3Flots.append(d3Flot.toDict())

    # fill notebook cells with flot + report links code
    try:
      cellCode = flotCode.format(
        name=cell.flot.title, description=cell.flot.description,
        cellNum=cellNum, reportNum=reportNum + 1, linksCode=linksCode
      )
    except TypeError:
      typeErr = 'Number of placeholders in cell code string do not match the number of args supplied'
      LOGGER.exception(typeErr)
      raise InvariantViloation(typeErr)

    nb['cells'].append(
      nbf.new_code_cell(source=cellCode, metadata={
        'init_cell': True, 'hide_input': True, 'editable': True, 'deletable': True
      })
    )

  xpdf.commit()
  return cellNum, d3Flots

def buildInitCell(nb, numOfCategories, d3Flots, appName, runId):
  """
  Method to build the init cell which contains the intro,
   serialized transactions object and metadata for generating reports

  """
  from xpedite.jupyter.templates import loadInitCell
  initCode = loadInitCell()
  try:
    envLink = buildReportLink('envReport', Action.Load)
    initCode = initCode.format(
      envLink=envLink, appName=appName, categoryCount=numOfCategories + 1, runId=runId
    )
  except TypeError:
    typeErr = 'Number of placeholders in init code string do not match the number of args supplied'
    LOGGER.exception(typeErr)
    raise InvariantViloation(typeErr)

  nb['cells'] = [nbf.new_code_cell(source=initCode, metadata={'init_cell': True, 'isInit': '0xFFFFFFFFA5A55A5DUL',\
  'hide_input': True, 'editable': False, 'deletable': False,\
  'd3Flots': d3Flots})] + nb['cells']


def buildNotebook(appName, result, notebookPath, dataFilePath, runId):
  """
  Method to build .ipynb notebook with init code
   cell for profiles and one report cell per category.

  """
  begin = time.time()
  LOGGER.info('generating notebook %s -> ', os.path.basename(notebookPath))
  nb = nbf.new_notebook()
  numOfCategories, d3Flots = buildReportCells(nb, result, dataFilePath)
  buildInitCell(nb, numOfCategories, d3Flots, appName, runId)

  try:
    with open(notebookPath, 'w') as reportFile:
      nbformat.write(nb, reportFile)
    notebookSize = formatHumanReadable(os.path.getsize(notebookPath))
    elapsed = time.time() - begin
    LOGGER.completed('completed %s in %0.2f sec.', notebookSize, elapsed)
    return True
  except IOError:
    LOGGER.exception('Could not write to the notebook(.ipynb) file')
    return False

def launchJupyter(homeDir):
  """
  Method to set env vars for overriding jup config, adding
   python path and extensions, and finally launching jupyter

  """
  from xpedite.jupyter         import SHELL_PREFIX
  from xpedite.dependencies    import binPath
  LOGGER.info('')
  pyPath = os.path.dirname(binPath('python')) + os.pathsep + os.environ['PATH']
  initPath = os.path.dirname(__file__)
  runtimePath = tempfile.mkdtemp(prefix=SHELL_PREFIX, dir='/tmp')
  jupyterEnv = os.environ
  jupyterEnv[Context.xpediteHomeKey] = os.path.abspath(homeDir)
  jupyterEnv['JUPYTER_PATH'] = os.path.join(initPath, 'data/extensions/')
  jupyterEnv['JUPYTER_CONFIG_DIR'] = os.path.join(initPath, 'data/config/')
  jupyterEnv['JUPYTER_RUNTIME_DIR'] = runtimePath
  jupyterEnv['HOME'] = runtimePath
  jupyterEnv['XPEDITE_PATH'] = os.path.abspath(os.path.join(initPath, '../../'))
  jupyterEnv['PATH'] = pyPath
  jupyterBinary = binPath('jupyter')
  os.execle(jupyterBinary, 'Xpedite', 'nbclassic', '--no-browser', '--notebook-dir='+homeDir, jupyterEnv)

def validatePath(homeDir, reportName):
  """Validates the path to store xpedite notebook and data files"""
  from xpedite.jupyter import DATA_DIR, DATA_FILE_EXT, TEMP_PREFIX, NOTEBOOK_EXT
  if homeDir is None:
    homeDir = tempfile.mkdtemp(prefix=TEMP_PREFIX, dir='/tmp')
    LOGGER.warning('Xpedite home directory not found in profileInfo (using temp dir).\n'
      'To keep all reports in one place, set variable homeDir in profileInfo to a valid path.')

  dataDir = os.path.join(homeDir, DATA_DIR)
  notebookPath = '{}/{}{}'.format(homeDir, reportName, NOTEBOOK_EXT)
  dataFilePath = '{}/{}{}'.format(dataDir, reportName, DATA_FILE_EXT)
  if os.path.isfile(notebookPath) or os.path.isfile(dataFilePath):
    errMsg = """Can't overwirte existing files. check and remove
      \t\t 1. Notebook file - {}
      \t\t 2. Xpedite data file - {}""".format(notebookPath, dataFilePath)
    LOGGER.error(errMsg)
    raise Exception(errMsg)

  if not os.path.exists(dataDir):
    LOGGER.info('creating xpedite data directory %s', dataDir)
    os.makedirs(dataDir)
  return notebookPath, dataFilePath, homeDir

class Driver(object):
  """Xpedite driver to render profile results in jupyter shell"""

  @staticmethod
  def render(profileInfo, report, leanReports=None, cprofile=None): # pylint: disable=unused-argument
    """Runs a profile session and renders results in a jupyter shell"""
    from xpedite.jupyter.result import Result
    result = Result(report)
    notebookPath, dataFilePath, profileInfo.homeDir = validatePath(profileInfo.homeDir, result.reportName)
    if result.reportCells:
      rc = buildNotebook(profileInfo.appName, result, notebookPath, dataFilePath, result.runId)
      if cprofile:
        cprofile.disable()
      if rc:
        launchJupyter(profileInfo.homeDir)
    else:
      LOGGER.error('Aborting profile - no txn collected. Did you generate any transactions ?')
