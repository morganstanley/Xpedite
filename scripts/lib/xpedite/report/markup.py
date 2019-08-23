"""
This module provides markup, css selectors, javascripts for generation of HTML reports

Author: Manikandan Dhamodharan, Morgan Stanley

"""
import os
import six
from xpedite.dependencies import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.HTML, Package.Pygments)
from html import HTML # pylint: disable=wrong-import-position

def loadFile(path):
  """
  Loads contents of the given file

  :param path: Path of the file to load

  """
  with open(path, 'rb') as fileHandle:
    return six.ensure_str(fileHandle.read())

def formatList(inputList):
  """
  Formats a list of items to html unordered list

  :param inputList: list of items to be formatted

  """
  report = HTML()
  htmlList = report.ul
  for val in inputList:
    val = str(val)
    htmlList.li(val)
  return report

TABLE_ENV = 'tableEnv tablesorter'
TABLE_SUMMARY = 'tableSummary tablesorter'
TABLE_REPORT_CONTAINER = 'tableReportContainer'
TABLE_REPORT = 'tableReport tablesorter'
TABLE_ROW_NO = 'tableRowNo'
TABLE_ROW_DATA = 'tableRowData'
TABLE_PMU = 'pmu'
TD_PMU_NAME = 'pmn'
TD_PMU_VALUE = 'pmv'
TH_DEBUG = 'thDebug'
TD_DEBUG = 'tdDebug'
TD_KEY = 'tdKey'
TD_END = 'tdEnd'
TIME_POINT_STATS_TITLE = 'timePointStatsTitle'
TRIVIAL_STATS_TABLE = 'trivialStatsTable'
SELECTOR = 'selector'
TIME_POINT_STATS = 'timePointStats-{}'
POSITIVE_DELTA = 'positiveDelta'
NEUTRAL_DELTA = 'neutralDelta'
NEGATIVE_DELTA = 'negativeDelta'

DURATION_FORMAT = '{0:4,.3f}'
DURATION_FORMAT_1 = '{1:4,.3f}'
DURATION_FORMAT_2 = '{2:4,.3f}'
DELTA_FORMAT_1 = '{0:4,.0f}'
DELTA_FORMAT_2 = '{2:4,.0f}'

HTML_BEGIN_FMT = """
<!doctype html>
<html>
<head>
  <style> {style} </style>
  <script> {jquery} </script>
  <script> {bootstrap} </script>
  <script> {tipsy} </script>
  <script> {tablesorter} </script>
  <script> {flot} </script>
  <script> {floatThead} </script>
  <script> {xpedite} </script>
  <script>
  $(document).ready(function () {{
    jQuery('table.tableReport').tablesorter();
    $('.tableReport').floatThead({{
      position: 'fixed'
    }});
  }});
  </script>
</head>
<body id="xpedite-report">
"""
HTML_END = '</body></html>'

JS_PATH = os.path.join(os.path.dirname(__file__), '../../../jupyter/js')
STYLE_PATH = os.path.join(os.path.dirname(__file__), '../../../jupyter/config/custom')
STATIC_REPORT_STYLE = loadFile(os.path.join(STYLE_PATH, 'static.css'))
XPEDITE_STYLE = loadFile(os.path.join(STYLE_PATH, 'xpedite.css'))
CODE_STYLE = loadFile(os.path.join(STYLE_PATH, 'code.css'))
XPEDITE = loadFile(os.path.join(JS_PATH, 'xpedite.js'))
JQUERY = loadFile(os.path.join(JS_PATH, 'jquery-3.2.1.min.js'))
BOOTSTRAP = loadFile(os.path.join(JS_PATH, 'bootstrap.min.js'))
TIPSY = loadFile(os.path.join(JS_PATH, 'jquery.tipsy.js'))
TABLE_SORTER = loadFile(os.path.join(JS_PATH, 'jquery.tablesorter.min.js'))
FLOT = loadFile(os.path.join(JS_PATH, 'jquery.flot.min.js'))
VIZ = loadFile(os.path.join(JS_PATH, 'viz.v1.js'))
FLOAT_THEAD = loadFile(os.path.join(JS_PATH, 'jquery.floatThead.min.js'))

STYLE = STATIC_REPORT_STYLE + XPEDITE_STYLE + CODE_STYLE

HTML_BEGIN = HTML_BEGIN_FMT.format(
  style=STYLE, xpedite=XPEDITE, tipsy=TIPSY, tablesorter=TABLE_SORTER,
  flot=FLOT, floatThead=FLOAT_THEAD, jquery=JQUERY, bootstrap=BOOTSTRAP
)

ERROR_TEXT = '<div class="errorText">{}</div>'

def getDeltaMarkup(delta):
  """Returns sign of delta for elapsed tsc or pmc value"""
  return '+' if delta > 0 else ''

def getDeltaType(delta):
  """Returns type of delta for elapsed tsc or pmc value"""
  return POSITIVE_DELTA if delta > .05 else (NEGATIVE_DELTA if delta < -.05 else NEUTRAL_DELTA)
