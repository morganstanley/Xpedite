"""
Module to support xpedite plugin framework

This module implements logic to discover and load plugins.
Xpedite plugins must adhere to all of the following conventions
  1. The name of the python modules and packages must begin with xpedite_
  2. The package or module must implement xpedite_provides() method, returning a
     list of xpedite objects provided by the plugin
  3. The package or module must implement xpedite_instantiate() method, that will
     create an instance of the provisioned object

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import sys
import pkgutil
import logging
import importlib

LOGGER = logging.getLogger(__name__)

class PluggableObject(object):
  """Pluggable objects to override/enhance functionality behaviour of the profiler"""

  def __init__(self, objType, name, module):
    self.objType = objType
    self.name = name
    self.module = module

  def instantiate(self):
    """Constructs and returns an instance of a pluggable object"""
    objFactory = getattr(self.module, 'xpedite_instantiate', None)
    if objFactory:
      return objFactory(self.objType, self.name)
    return None

def loadPluggableObjects():
  """
  Loads modules providing Xpedite pluggable objects


  """
  pluginPath = os.environ.get('XPEDITE_PLUGIN_PATH')
  if pluginPath:
    sys.path.append(pluginPath)
  pluggableObjectsMap = {}
  for _, name, _ in pkgutil.iter_modules():
    try:
      if name.startswith('xpedite_'):
        module = importlib.import_module(name)
        manifestCb = getattr(module, 'xpedite_provides', None)
        if manifestCb:
          for objType, objName in manifestCb():
            LOGGER.debug('%s provides pluggable object type - %s | name - %s', name, objType, objName)
            pluggableObjectsMap.setdefault(objType, []).append(PluggableObject(objType, objName, module))
    except Exception as ex:
      LOGGER.debug('Failed to load plugin %s - %s', name, ex)
  return pluggableObjectsMap

PLUGGABLE_OBJECTS_MAP = loadPluggableObjects()

def loadObject(objType, objName=None, default=None):
  """
  Loads object of a given type from plugins

  :param objType: Type of the object to load (Default value = None)
  :param objName: Name of the object to load(Default value = None)
  :param default: Default instance, for objects not provided by any plugins (Default value = None)

  """
  pluggableObjects = PLUGGABLE_OBJECTS_MAP.get(objType)
  if pluggableObjects:
    index = pluggableObjects.index(objName) if objName and objName in pluggableObjects else 0
    return pluggableObjects[index].instantiate()
  return default
