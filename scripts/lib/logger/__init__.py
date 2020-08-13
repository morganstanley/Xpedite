"""
Utility module to override Python log formatting and print colored and formatted
log messages to the console.
This module formats log levels in the logging module, as well as logs for completed actions,
actions that have timed out, and a level used to write only to an Xpedite log file

Author: Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
import logging
import datetime
import types

LOG_CONFIG_PATH = os.path.join(os.path.dirname(__file__), 'conf.ini')

def init():
  import logging.config
  logging.config.fileConfig(LOG_CONFIG_PATH)

def enableVerboseLogging():
  """Set console handler level to debug to display information in verbose mode"""
  logger = logging.getLogger('xpedite')
  for handler in logger.handlers:
    if isinstance(handler, ConsoleHandler):
      handler.setLevel(logging.getLevelName(logging.DEBUG))

class ConsoleFormatter(logging.Formatter):
  """Format log messages to display in the console"""
  textColor = {
    'WARNING': 'yellow',
    'ERROR': 'red',
    'COMPLETED': 'green',
    'TRACE': 'blue',
  }

  lineCount = 0
  completionLogCount = 0

  def format(self, record):
    """Format log messages with level name and color"""
    from xpedite.dependencies import Package, DEPENDENCY_LOADER
    DEPENDENCY_LOADER.load(Package.Termcolor)
    from termcolor import colored

    msg = super(ConsoleFormatter, self).format(record)
    if record.levelname == 'COMPLETED':
      prefix = '\n' if ConsoleFormatter.completionLineCount > 0 else ''
      msg = prefix + msg
      ConsoleFormatter.completionLineCount += 1
    else:
      prefix = '\n' if ConsoleFormatter.lineCount > 0 else ''
      if record.levelname == 'ERROR' or record.levelname == 'WARNING':
        msg = '{}[{}]: {}'.format(prefix, record.levelname, msg)
      else:
        msg = prefix + msg
      ConsoleFormatter.completionLineCount = 0

    ConsoleFormatter.lineCount += 1
    if record.levelname in self.textColor:
      return colored(msg, self.textColor[record.levelname])
    return msg

class FileFormatter(logging.Formatter):
  """Format log messages to write to log file"""
  def format(self, record):
    """Format log messages with timestamp, file name, and line no"""
    msg = super(FileFormatter, self).format(record)
    timestamp = datetime.datetime.fromtimestamp(record.created).strftime('%Y-%m-%d %H:%M:%S')
    prefix = '{} - [{}:{}]'.format(timestamp, record.filename, record.lineno)
    msg = '{0:-<50} {1:}'.format(prefix, msg)
    return msg

class FileHandler(logging.FileHandler):
  """Create a custom handler to write to an xpedite log file"""
  def __init__(self, mode):
    from xpedite.util import makeLogPath
    path = '{}/xpedite.log'.format(makeLogPath())
    logging.FileHandler.__init__(self, path, mode)

class ConsoleHandler(logging.StreamHandler):
  """Create a custom handler to format log messages using a custom edit method to get rid of newlines"""
  def __init__(self):
    logging.StreamHandler.__init__(self)

  def emit(self, record):
    """Create a custom emit method to get rid of newlines in log messages"""
    try:
      msg = self.format(record)
      if not hasattr(types, 'UnicodeType'):
        self.stream.write(msg)
      else:
        try:
          if getattr(self.stream, 'encoding', None) is not None:
            self.stream.write(msg.encode(self.stream.encoding))
          else:
            self.stream.write(msg)
        except UnicodeError:
          self.stream.write(msg.encode('UTF-8'))
      self.flush()
    except Exception(KeyboardInterrupt, SystemExit) as ex:
      raise ex
    except Exception as ex:
      self.handleError(record)

COMPLETED_LEVEL_NUM = 25
def completed(self, message, *args, **kws):
  """Add support to log messages for completed actions in green"""
  self._log(COMPLETED_LEVEL_NUM, message, args, **kws) # pylint: disable=protected-access

logging.addLevelName(COMPLETED_LEVEL_NUM, 'COMPLETED')
logging.Logger.completed = completed

TRACE_LEVEL_NUM = 5
def trace(self, message, *args, **kws):
  """Add support to log messages for trace"""
  self._log(TRACE_LEVEL_NUM, message, args, **kws) # pylint: disable=protected-access

logging.addLevelName(TRACE_LEVEL_NUM, 'TRACE')
logging.Logger.trace = trace
