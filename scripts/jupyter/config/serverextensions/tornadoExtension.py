"""
Tornado extension to redirect and serve http requests for Xpedite
static html and data files

Author: Dhruv Shekhawat, Morgan Stanley
"""

from tornadoExtension        import *
from notebook.utils          import url_path_join
from notebook.base.handlers  import IPythonHandler
from tornado                 import template
import tornado.web
import json
import zlib
import base64
import os
import sys

class HtmlReportHandler(tornado.web.RequestHandler):
  """Class to serve html reports through links with
  query params as notebook path and (cellId, reportId)
  as indices to read metadata from notebook
  """
  def get(self):
    xpeditePath = os.path.normpath(os.path.join(__file__, '../../../../lib'))
    sys.path.append(xpeditePath)

    from xpedite.jupyter import buildXpdName
    from xpedite.jupyter.xpediteData import XpediteDataReader
    from xpedite.jupyter.context import Context

    try:
      action = self.get_argument('action', None)
      reportKey = self.get_argument('reportKey', None)
      assert reportKey is not None
      xpdFileName = buildXpdName(self.get_argument(Context.fileKey, None))
      assert xpdFileName is not None
      xpDataPath = os.path.join(Context.xpediteDataPath, xpdFileName)
      with XpediteDataReader(xpDataPath) as xpd:
        data = xpd.getData(reportKey)
        markupGz = base64.b64decode(data)
        self.set_header("Content-type", 'text/html')
        self.set_header("Content-Encoding", 'gzip')
        self.finish(markupGz)
    except IOError:
      ioErr = 'Could not read html content from nbPath parameter. Check if file exists.'
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

def load_jupyter_server_extension(nb_server_app):
  """Method called first to load the
  server extension on jupyter startup
  """
  web_app = nb_server_app.web_app
  host_pattern = '.*$'
  route_pattern = url_path_join(web_app.settings['base_url'], '/xpedite')
  web_app.add_handlers(host_pattern, [(route_pattern, HtmlReportHandler)])
