#!/usr/bin/env python
"""
Database for supported pmc events for a cpu micro architecture

This module provides
  1. A database to store and lookup pmc events for each of the supported micro architectures
  2. A container to cache EventsDb objects

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import re
import sys
import copy
import logging

LOGGER = logging.getLogger(__name__)

class CmaskQualifier(object):
  """Predicate to process cmask qualified event names"""

  # regex to decode cmask qualified event names
  regEx = re.compile(r'c(?:mask=)?(0x[0-9a-f]+|[0-9]+)')

  def __init__(self, counterMask):
    self.counterMask = counterMask

  @staticmethod
  def match(qualifier):
    """Detects the presence of cmask qualifier in event name"""
    reMatch = re.match(CmaskQualifier.regEx, qualifier)
    if reMatch:
      return CmaskQualifier(int(reMatch.group(1), 16))
    return None

  def apply(self, event):
    """Method to set cmask bits"""
    event.counterMask = self.counterMask
    return event

class EventsDb(object):
  """A database of all known pmu events for a cpu micro architecture"""

  regEx = re.compile(r'(.*?):(.*)')

  def __init__(self, uarchSpec, eventsMap):
    self.uarchSpec = uarchSpec
    self.eventsMap = eventsMap

  @staticmethod
  def stripQualifier(eventName):
    """Strips qualifiers from event name"""
    qualifiers = None
    reMatch = re.match(EventsDb.regEx, eventName)
    if reMatch:
      eventName = reMatch.group(1)
      qualifiers = reMatch.group(2)
    return eventName, qualifiers

  @staticmethod
  def applyQualifiers(event, qualifiers):
    """Sets the qualifier bit in select bitmask of a pmu event"""
    qualifier = CmaskQualifier.match(qualifiers)
    if qualifier:
      return qualifier.apply(event)
    return None

  def uarchName(self):
    """Returns  micro architecture name of the events database"""
    return self.uarchSpec.name

  def topdownRatios(self):
    """Returns topdown hierarchy for this events database"""
    return self.uarchSpec.topdownRatios()

  def __contains__(self, eventName):
    if eventName in self.eventsMap:
      return True
    name, qualifiers = self.stripQualifier(eventName)
    return name in self.eventsMap and CmaskQualifier.match(qualifiers)

  def __getitem__(self, eventName):
    """Returns pmu event for the given name"""
    if eventName not in self.eventsMap:
      name, qualifiers = self.stripQualifier(eventName)
      event = self.eventsMap[name]
      if event:
        event = copy.deepcopy(event)
        event.name = eventName
        return self.applyQualifiers(event, qualifiers)
    return self.eventsMap[eventName]

  def __len__(self):
    """Returns the number of events in the events database"""
    return len(self.eventsMap)

  def __repr__(self):
    """Returns string representation of this EventsDb"""
    eventsDbStr = ''
    for event in self.eventsMap.values():
      eventsDbStr += '{}\n'.format(event)
    return eventsDbStr

class EventsDbCache(object):
  """A cache for event databases for all known cpu micro architectures"""

  def __init__(self):
    self.cache = {}

  def get(self, cpuId):
    """
    Returns events database for the given cpu model

    :param cpuId: Id of cpu to lookup

    """
    if cpuId in self.cache:
      return self.cache[cpuId]
    eventsDb = loadEventsDb(cpuId)
    self.cache.update({cpuId : eventsDb})
    return eventsDb

def loadEventsDb(cpuId):
  """
  Loads events database for a cpu model

  :param cpuId: Id of cpu to load

  """
  from xpedite.pmu.eventsLoader import EventsLoader
  loader = EventsLoader()

  from xpedite.pmu.uarchspec .uarchSpecLoader import loadUarchSpecDb
  uarchSpec = loadUarchSpecDb().spec(cpuId)
  if not uarchSpec:
    raise Exception(
      'failed to locate events database file for cpu {}. please consult xpedite devs '
      'to add support for this architecture'.format(cpuId)
    )
  eventsMap = loader.loadJson(uarchSpec.coreEventsDbFile)
  if len(eventsMap) <= 0:
    raise Exception('failed to load events database from file [{}]'.format(uarchSpec.coreEventsDbFile))
  return EventsDb(uarchSpec, eventsMap)

def main():
  """Displays pmu events database for localhost"""
  from xpedite.util import getCpuId
  eventsDb = loadEventsDb(getCpuId())
  LOGGER.info('\n%s', eventsDb)

if __name__ == '__main__':
  sys.exit(not main())
