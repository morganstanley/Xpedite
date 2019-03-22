"""
Probe definitions

This module provides a factory to build probes

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from xpedite.types import InvariantViloation
from xpedite.types.probe import AnchoredProbe
from xpedite.types.containers import ProbeMap
from xpedite.types.route import Route

class ProbeFactory(object):
  """Utility class to intern probe generation"""

  FIELD_DELIMITER = ' | '
  KEY_VALUE_DELIMITER = '='
  FIELD_RECORDER_RETURN_SITE = 'RecorderReturnSite'
  FIELD_NAME = 'Name'
  FIELD_FILE = 'File'
  FIELD_LINE = 'Line'
  FIELD_ATTRIBUTES = 'Attributes'
  FIELD_STATUS = 'Status'
  PROBE_STATUS_ENABLED = 'enabled'

  def __init__(self, workspace=None):
    self.workspace = workspace

  cache = {}
  def buildProbe(self, name, filePath, lineNo, sysName):
    """
    Builds an instance of probe using fly weight pattern

    :param name: Name of the probe
    :param filePath: Path of the source file containing the probe
    :param lineNo: Line number of the statement containing the probe
    :param sysName: Instrumented name of the probe

    """
    key = (sysName, filePath, lineNo)
    if key in ProbeFactory.cache:
      return ProbeFactory.cache[key]
    filePath = self.trimWorkspace(filePath, self.workspace)
    probe = AnchoredProbe(name, filePath, lineNo, '0', sysName)
    ProbeFactory.cache.update({key:probe})
    return probe

  @staticmethod
  def trimWorkspace(filePath, workspace):
    """
    Trims the build prefix from source code path

    :param path: path to the source file

    """
    filePath = filePath[len(workspace):] if workspace and filePath.startswith(workspace) else filePath
    filePath = filePath[1:] if filePath.startswith('/') else filePath
    return filePath

  def buildFromRecords(self, records):
    """
    Parses and loads list of instrumented probes from probe records

    :param records: records from the appInfo file

    """
    probes = {}
    for record in records:
      fields = {}
      for field in record.split(self.FIELD_DELIMITER):
        index = field.find(self.KEY_VALUE_DELIMITER)
        if index == -1 or len(field) < (index+1):
          raise InvariantViloation('detected invalid probe record in app info file - {}'.format(record))
        fields.update({field[:index]:field[index+1:]})
      if fields:
        try:
          fields[self.FIELD_FILE] = self.trimWorkspace(fields[self.FIELD_FILE], self.workspace)
          probes.update({
            fields[self.FIELD_RECORDER_RETURN_SITE] : AnchoredProbe(
              fields[self.FIELD_NAME], fields[self.FIELD_FILE], fields[self.FIELD_LINE],
              fields[self.FIELD_ATTRIBUTES], fields[self.FIELD_STATUS] == self.PROBE_STATUS_ENABLED,
              fields[self.FIELD_NAME]
            )
          })
        except KeyError as error:
          raise InvariantViloation('detected record missing field {} - \n{}\n{}'.format(error, record, fields))
    return probes

class ProbeIndexFactory(object):

  """Utility class to intern probe map generation"""

  class Index(object):
    """probe based index - route and probe map"""

    def __init__(self, route, probeMap):
      self.route = route
      self.probeMap = probeMap

  cache = {}

  @staticmethod
  def _addCounterToMap(probeMap, counter, index):
    """
    Inserts or updates the gvien counter to the probe map

    :param counter: Counter to be added
    :param index: relative index for probes, that got hit many times

    """
    if counter.probe in probeMap:
      probeMap[counter.probe].append(index)
    else:
      probeMap.update({counter.probe : [index]})

  @staticmethod
  def buildIndex(counters):
    """
    Builds an instance of probe map using fly weight pattern

    :param counters: A collection of counters

    """
    route = Route((counter.probe for counter in counters))
    index = ProbeIndexFactory.cache.get(route, None)
    if not index:
      probeMap = ProbeMap()
      for i, counter in enumerate(counters):
        ProbeIndexFactory._addCounterToMap(probeMap, counter, i)
      index = ProbeIndexFactory.Index(route, probeMap)
      ProbeIndexFactory.cache.update({route : index})
    return index

  @staticmethod
  def reset():
    """Resets the internal probe index cache"""
    ProbeIndexFactory.cache = {}
