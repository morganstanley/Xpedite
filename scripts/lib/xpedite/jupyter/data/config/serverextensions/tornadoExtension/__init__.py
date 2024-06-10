"""
Tornado extension to redirect and serve http requests for Xpedite
static html and data files

Author: Dhruv Shekhawat, Morgan Stanley
"""

#from notebook.utils          import url_path_join
import jupyter_server
from jupyter_server.base.handlers import JupyterHandler
import tornado
from tornado                 import template
import tornado.web
import json
import zlib
import base64
import os
import sys

class HtmlReportHandler(JupyterHandler):
  """Class to serve html reports through links with
  query params as notebook path and (cellId, reportId)
  as indices to read metadata from notebook
  """
  @tornado.web.authenticated
  def get(self):
    xpeditePath = os.path.normpath(os.path.join(__file__, '../../../../../../..'))
    sys.path.append(xpeditePath)

    from xpedite.jupyter.xpediteData import XpediteDataReader
    from xpedite.jupyter.context import Context

    try:
      action = self.get_argument('action', None)
      reportKey = self.get_argument('reportKey', None)
      assert reportKey is not None
      notebookPath = self.get_argument(Context.notebookPathKey, None)
      assert notebookPath is not None
      xpdFilePath = Context.buildXpdPath(notebookPath)
      with XpediteDataReader(xpdFilePath) as xpd:
        data = xpd.getData(reportKey)
        markupGz = base64.b64decode(data)
        self.set_header("Content-type", 'text/html')
        self.set_header("Content-Encoding", 'gzip')
        self.finish(markupGz)
    except IOError:
      ioErr = 'Could not read html from xpd file - {} with key - {}'.format(xpdFilePath, reportKey)
      self.finish(ioErr)
      print(ioErr)
    except AssertionError as e:
      assertErr = 'Fatal error - The request is missing mandatory query parameters.'
      self.finish(assertErr)
      print(assertErr)

def get_init_cell(jsonReport):
  """returns cell with init metadata
  """
  for cell in jsonReport['cells']:
    metadata = cell['metadata']
    if(('isInit' in metadata) and (metadata['isInit'] == '0xFFFFFFFFA5A55A5DUL')):
      return cell

def _load_jupyter_server_extension(serverapp: jupyter_server.serverapp.ServerApp):
    """
    This function is called when the extension is loaded.
    """
    handlers = [('/xpedite', HtmlReportHandler)]
    serverapp.web_app.add_handlers(".*$", handlers)

def _jupyter_server_extension_points():
    """
    Returns a list of dictionaries with metadata describing
    where to find the `_load_jupyter_server_extension` function.
    """
    return [{"module": "tornadoExtension"}]
