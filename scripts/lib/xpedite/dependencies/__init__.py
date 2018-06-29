"""
Package to manage xpedite dependencies

Provides a enumeration for a list of Xpedite dependencies (Package)

The module also implements a plugin framework, to expose hook for loading of
dependencies and micro architectural specifications.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

class Dependency(object):
  """Class to model xpedite dependency on a python package"""

  def __init__(self, name, version, required):
    self.name = name
    self.version = version
    self.required = required
    self.isAvailable = None

  def __repr__(self):
    return '{} version {} - required = {}'.format(self.name, self.version, self.required)

class Package(object):
  """Enumeration of python packages needed by Xpedite"""

  Enum = Dependency('enum34', '1.1.6', True)
  FuncTools = Dependency('functools32', '3.2.3-1', True)
  Futures = Dependency('futures', '2.1.6', True)
  HTML = Dependency('html', '1.16', True)
  Netifaces = Dependency('netifaces', '0.10.4-py27', True)
  Numpy = Dependency('numpy', '1.6.1-mkl', True)
  Pygments = Dependency('pygments', '2.0.2-py27', True)
  Rpyc = Dependency('rpyc', '3.3.0-py27', False)
  Cement = Dependency('cement', '2.8.2', True)
  Termcolor = Dependency('termcolor', '1.1.0', True)
  PyCpuInfo = Dependency('py-cpuinfo', '0.1.2', True)

  # needed by jupyter nb format - 'Futures', 'NbFormat', 'JsonSchema', 'FuncTools',
  #'Traitlets', 'Six', 'Notebook', 'Tornado', 'IPythonGenUtils'
  IPythonGenUtils = Dependency('ipython_genutils', '0.1.0', True)
  JsonSchema = Dependency('jsonschema', '2.5.0', True)
  NbFormat = Dependency('nbformat', '4.4.0', True)
  Notebook = Dependency('notebook', '4.3.1-py27', True)
  Six = Dependency('six', '1.10.0', True)
  Tornado = Dependency('tornado', '4.4.2-py27', True)
  Traitlets = Dependency('traitlets', '4.3.1-py27', True)

def buildDependencyLoader():
  """Builds an instance of dependency loader"""
  from xpedite.dependencies.pluginLoader import loadObject
  loader = loadObject('dependencyLoader')
  if loader:
    return loader
  class DefaultDependencyLoader(object):
    """Default dependency loader implementation"""

    @staticmethod
    def load(*_):
      """Default nop load operation"""
      return True
  return DefaultDependencyLoader()

def buildConfig():
  """Loads xpedite configurations """
  from xpedite.dependencies.pluginLoader import loadObject
  from xpedite.dependencies.config import Config
  config = loadObject('config', default={})
  return Config(config)

CONFIG = buildConfig()

DEPENDENCY_LOADER = buildDependencyLoader()

def loadDriver(name=None):
  """
  Loads driver for the current profile session

  A driver implements interface for rendering profiling results

  :param name: Name of the Xpedite driver (Default value = None)

  """
  if name:
    from xpedite.dependencies.pluginLoader import loadObject
    driver = loadObject('driver')
    if driver:
      return driver

def binPath(binaryName):
  """
  Returns path of an executable with the given name

  :param binaryName: Name of the executable

  """
  import os
  from distutils.spawn import find_executable
  envVariable = 'XPEDITE_{}'.format(binaryName.upper())
  if envVariable in os.environ.keys():
    return os.environ[envVariable]
  else:
    executablePath = find_executable(binaryName)
    if executablePath:
      return executablePath
    else:
      raise Exception('Failed to find binary {} in path.'.format(binaryName))
