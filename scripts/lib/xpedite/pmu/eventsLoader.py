#!/usr/bin/env python2.7
"""
Loader to build an events database from micro architecture specifications (json)

This module provides factories to parse and build event objects
Each record is classified into one of following types
  1. generic core - events programmable in general purpose pmu registers
  2. fixed core   - events colleted by fixed pmu registers
  3. offcore      - counters for offcore requests and responses

Each loaded record is mapped to a factory object based on its type.
The factory will inturn construct events corresponding to record type.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import logging
from xpedite.pmu.uarchEvent  import GenericCoreEvent, FixedCoreEvent, OffCoreEvent

LOGGER = logging.getLogger(__name__)

class AttrInitializer(object):
  """Predicate to transform and initialize uarch event attributes"""

  def __init__(self, name, converter):
    self.name = name
    self.converter = converter

  def initialize(self, obj, value):
    """Applies a converter and sets attribute of the given uarch event"""
    setattr(obj, self.name, self.converter(value))

class ObjectFactory(object):
  """Factory to load and set uarch event attributes from cpu micro architecture specifications"""

  def __init__(self):
    self.attrMap = {}

  def add(self, fieldName, attrName, converter):
    """
    Associates an attribute with field name for a given  micro architecture spec record

    :param fieldName: Name of the field in micro architecture spec records
    :param converter: Predicate to apply transformations to raw value from records
    :param attrName: Name of the attribute to be set by the newly added initializer

    """
    self.attrMap.update({fieldName : AttrInitializer(attrName, converter)})

  def build(self, obj, record):
    """Sets attributes of an uarch event with values from given record"""
    for fieldName, value in record.iteritems():
      if fieldName in self.attrMap:
        self.attrMap[fieldName].initialize(obj, value)
    return obj

  def __repr__(self):
    return str(self.attrMap)

class EventsLoader(object):
  """Loads events from micro archtitecture specifications"""

  # pylint: disable=bad-whitespace

  @staticmethod
  def loadCsv(eventsFile):
    """
    Loads micro architecture specs from a csv file

    :param eventsFile: csv file with uarch spec data

    """
    import csv
    factory = ObjectFactory()
    factory.add('EventSelect', 'eventSelect', lambda v : int(v, 16))
    factory.add('UnitMask', 'unitMask', lambda v : int(v, 16))
    factory.add('Description', 'description', lambda v : v)

    eventsMap = {}
    with open(eventsFile) as eventsFileHandle:
      reader = csv.DictReader([row for row in eventsFileHandle if row[0]!='#'])
      for record in reader:
        eventName = record['EventName']
        if eventName:
          event = GenericCoreEvent()
          factory.build(event, record)
          eventsMap.update({eventName : event})
    return eventsMap

  @staticmethod
  def decodePmcList(pmcListStr):
    """Converts a comma seperated list of values to a set"""
    pmcListStr = pmcListStr.strip('"')
    return set(int(pmc) for pmc in pmcListStr.split(','))

  @staticmethod
  def jsonGenericCoreFactory():
    """Builds a factory for creating generic uarch events"""
    factory = ObjectFactory()
    factory.add(u'EventName',          'name',             lambda v : v)
    factory.add(u'EventCode',          'eventSelect',      lambda v : int(v, 16))
    factory.add(u'UMask',              'unitMask',         lambda v : int(v, 16))
    factory.add(u'CounterMask',        'counterMask',      int)
    factory.add(u'Invert',             'invert',           lambda v : int(v) != 0)
    factory.add(u'BriefDescription',   'briefDescription', lambda v : v if v else 'unknown')
    factory.add(u'PublicDescription',  'description',      lambda v : v if v else 'unknown')
    factory.add(u'Counter',            '_validSmtPmc',     EventsLoader.decodePmcList)
    factory.add(u'CounterHTOff',       '_validPmc',        EventsLoader.decodePmcList)
    factory.add(u'MSRIndex',           'msrIndex',         lambda v : v)
    factory.add(u'MSRValue',           'msrValue',         lambda v : int(v, 16))
    factory.add(u'AnyThread',          'anyThread',        int)
    factory.add(u'EdgeDetect',         'edgeDetect',       lambda v : int(v) != 0)
    factory.add(u'PEBS',               'pebs',             lambda v : int(v) != 0)
    factory.add(u'TakenAlone',         'takenAlone',       lambda v : int(v) != 0)
    factory.add(u'Data_LA',            'dataLA',           lambda v : int(v) != 0)
    factory.add(u'L1_Hit_Indication',  'l1HitIndication',  lambda v : int(v) != 0)
    factory.add(u'Errata',             'errata',           lambda v : v)
    return factory

  fixedCounterPrefix = 'Fixed counter '
  def jsonFixedCoreFactory(self):
    """Builds a factory for creating fixed uarch events"""
    factory = self.jsonGenericCoreFactory()
    factory.add(u'Counter', '_validSmtPmc', lambda v : int(v.split(EventsLoader.fixedCounterPrefix)[1]))
    factory.add(u'CounterHTOff', '_validPmc', lambda v : int(v.split(EventsLoader.fixedCounterPrefix)[1]))
    return factory

  def jsonOffCoreFactory(self):
    """Builds a factory for creating offcore uarch events"""
    factory = self.jsonGenericCoreFactory()
    factory.add(u'EventCode', 'eventSelect', lambda v : [int(ec, 16) for ec in v.split(',')])
    return factory

  def jsonFactory(self, record):
    """Builds a factory based on the type of the record"""
    offcoreFlag = record[u'Offcore'] if u'Offcore' in record else 0
    if int(offcoreFlag) != 0 or record[u'EventName'] == 'OFFCORE_RESPONSE':
      factory = self.jsonOffCoreFactory()
      return lambda r : factory.build(OffCoreEvent(), r)
    elif record[u'Counter'].startswith(EventsLoader.fixedCounterPrefix):
      factory = self.jsonFixedCoreFactory()
      return lambda r : factory.build(FixedCoreEvent(), r)
    factory = self.jsonGenericCoreFactory()
    return lambda r : factory.build(GenericCoreEvent(), r)

  @staticmethod
  def isOffcoreEventRecord(record):
    """Checks if a uarch spec record requires programming offcore registers"""
    offcoreFlag = record[u'Offcore'] if u'Offcore' in record else 0
    return int(offcoreFlag) != 0 or record[u'EventName'] == 'OFFCORE_RESPONSE'

  def loadJson(self, eventsFile):
    """
    Loads uarch spec from json files

    :param eventsFile: File to load uarch spec from

    """
    import json
    eventsDb = {}
    incompatibleRecords = []
    uninitializedEvents = []
    try:
      records = json.load(open(eventsFile, 'rb'))
      for record in records:
        try:
          event = self.jsonFactory(record)(record)
          if event.unInitialized():
            uninitializedEvents.append(event)
            continue
        except Exception as ex:
          line = '\n{}\n'.format('-' * 80)
          LOGGER.debug('%s failed to load record %s | %s%s', line, record, ex, line)
          incompatibleRecords.append(record)
          continue
        eventsDb.update({event.name : event})
    except ValueError as ex:
      raise Exception('failed to load pmu uarch events db from json file ({}) - {}'.format(eventsFile,ex))
    if uninitializedEvents:
      raise Exception('failed to initialize {} events'.format(len(uninitializedEvents)))
    if incompatibleRecords:
      raise Exception('failed to load {} uarch event records'.format(len(incompatibleRecords)))
    return eventsDb
