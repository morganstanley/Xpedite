"""
Package to manage xpedite dependencies

Provides a enumeration for a list of Xpedite dependencies (Package)

The module also implements a plugin framework, to expose hook for loading of
dependencies and micro architectural specifications.

Author: Manikandan Dhamodharan, Morgan Stanley
"""
import os
import re

def loadRequirements():
  """Loads a map of minimum version for all required dependencies"""
  requirements = {}
  reqFile = os.path.join(os.path.dirname(__file__), '../requirements.txt')
  with open(reqFile) as reqFileHandle:
    for record in reqFileHandle:
      fields = re.split('>=|==', record)
      if fields and len(fields) >= 2:
        name = fields[0].strip()
        minVersion = fields[1].strip()
        requirements.update({name:minVersion})
  return requirements

class Dependency(object):
  """Class to model xpedite dependency on a python package"""

  requirements = loadRequirements()

  def __init__(self, name, required, minVersion=None):
    self.name = name
    self.required = required
    self.minVersion = minVersion
    self.isAvailable = None

  def __repr__(self):
    return '{} | minVersion - {} | required = {}'.format(self.name, self.minVersion, self.required)

  @staticmethod
  def get(name, required):
    """Returns an instance of dependency"""
    return Dependency(name, required, Dependency.requirements.get(name, None))

class Package(object):
  """Enumeration of python packages needed by Xpedite"""

  Enum = Dependency.get('enum34', True)
  FuncTools = Dependency.get('functools32', True)
  Futures = Dependency.get('futures', True)
  Netifaces = Dependency.get('netifaces', True)
  Numpy = Dependency.get('numpy', True)
  Pygments = Dependency.get('pygments', True)
  Rpyc = Dependency.get('rpyc', False)
  Cement = Dependency.get('cement', True)
  Termcolor = Dependency.get('termcolor', True)
  PyCpuInfo = Dependency.get('py-cpuinfo', True)
  Jupyter = Dependency.get('jupyter', True)
  Six = Dependency.get('six', True)

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
  return None

def binPath(binaryName):
  """
  Returns path of an executable with the given name

  :param binaryName: Name of the executable

  """
  from distutils.spawn import find_executable # pylint: disable=no-name-in-module,import-error
  envVariable = 'XPEDITE_{}'.format(binaryName.upper())
  if envVariable in os.environ:
    return os.environ[envVariable]
  executablePath = find_executable(binaryName)
  if executablePath:
    return executablePath
  raise Exception('Failed to find binary {} in path.'.format(binaryName))
