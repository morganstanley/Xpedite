#!/usr/bin/env python
"""
Classes to construct and store pmc requests

This module defines generic, fixed and offcore objects to store different pmc attributes
It also provide a factory to transform a pmc event to corresponding request object.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import struct
import copy
from xpedite.pmu.uarchEvent   import GenericCoreEvent, FixedCoreEvent, OffCoreEvent
from xpedite.pmu.allocator    import Allocator

class GenericPmuRequest(object):
  """Request to program general purpose pmu registers"""

  def __init__(self, name, event, user=True, kernel=True):
    self.name = name
    self.uarchName = event.name
    if not name:
      raise Exception('Event must have a name - failed to create an event with no name')

    self.event = event
    if not event:
      raise Exception('failed to build general purpose pmu event request - invalid micro architectural event')

    self.user = user
    self.kernel = kernel
    if not self.user and not self.kernel:
      raise Exception(
        'failed to build general purpose pmu event request - Invalid flags for user/kernel (must enable at least one)'
      )

  def buildMask(self):
    """Builds bit mask for general purpose pmu request"""
    return struct.pack('=BBBBBBBB',
      self.event.eventSelect,
      self.event.unitMask,
      1 if self.user else 0,
      1 if self.kernel else 0,
      self.event.invert,
      self.event.counterMask,
      self.event.edgeDetect,
      self.event.anyThread)

  @staticmethod
  def defaultMask():
    """Default bit mask for general purpose pmu request"""
    return struct.pack('=BBBBBBBB', 0, 0, 0, 0, 0, 0, 0, 0)

  def __repr__(self):
    """Returns string representation of this generic pmu request"""
    flags = None
    if self.user and self.kernel:
      flags = 'user|kernel'
    else:
      flags = 'user' if self.user else 'kernel'
    return '{} [{}]'.format(self.name, flags)

class OffcorePmuRequest(object):
  """Request to program offcore pmu registers"""

  def __init__(self, name, event):
    self.name = name
    self.uarchName = event.name
    self.event = event
    if not name:
      raise Exception('Event must have a name - failed to create an event with no name')

  def buildMask(self):
    """Builds bit mask for offcore pmu request"""
    return struct.pack('=Q', self.event.msrValue)

  @staticmethod
  def defaultMask():
    """Default bit mask for offcore pmu request"""
    return struct.pack('=Q', 0)

  def __repr__(self):
    """Returns string representation of this offcore pmu request"""
    return '{}'.format(self.name)

class FixedPmuRequest(object):
  """Request to program fixed pmu registers"""

  def __init__(self, name, event, user=True, kernel=True):
    self.name = name
    self.uarchName = event.name
    self.ctrIndex = event.validPmc
    self.user = user
    self.kernel = kernel
    if not self.user and not self.kernel:
      raise Exception(
        'failed to build fixed pmu event request - Invalid flags for user/kernel (must enable at least one)'
      )

  def buildMask(self):
    """Builds bit mask for fixed pmu request"""
    return struct.pack('=BBB', self.ctrIndex,
        1 if self.user else 0,
        1 if self.kernel else 0)

  @staticmethod
  def defaultMask():
    """Default bit mask for fixed pmu request"""
    return struct.pack('=BBB', 0, 0, 0)

  def __repr__(self):
    """Returns string representation of a this fixed purpose pmu request"""
    flags = None
    if self.user and self.kernel:
      flags = 'user|kernel'
    else:
      flags = 'user' if self.user else 'kernel'
    return 'Fixed PMU Request {} [{}]'.format(self.ctrIndex, flags)

class PmuRequestFactory(object):
  """Factory to build requests for programming pmu events"""

  def __init__(self, eventsDb):
    self.eventsDb = eventsDb
    self.eventTypeMap = {
      # disable pylint warnings for unnecessary lambdas (W0108)
      GenericCoreEvent.eventType : lambda n, e, user, kernel: [GenericPmuRequest(n, e, user, kernel)], # pylint: disable=W0108
      FixedCoreEvent.eventType   : lambda n, e, user, kernel: [FixedPmuRequest(n, e, user, kernel)], # pylint: disable=W0108
      OffCoreEvent.eventType     : self.buildOffCoreRequests,
    }
    self.offCoreEventCount = 0

  def buildOffCoreRequests(self, name, event, user, kernel):
    """
    Builds requests to program offcore pmu registers

    :param name: Name of the event
    :param event: Offcore pmu event
    :param user: Flag to enable pmu event in user space
    :param kernel: Flag to enable pmu event in kernel space

    """
    if self.offCoreEventCount >= len(OffCoreEvent.eventSelects):
      raise Exception(
        'pmu request exceeds the max number of supported offCore events {}'.format(len(OffCoreEvent.eventSelects))
      )
    offcoreEvent = copy.deepcopy(event)
    offcoreEvent.eventSelect = OffCoreEvent.eventSelects[self.offCoreEventCount]
    self.offCoreEventCount += 1
    return [GenericPmuRequest(name, offcoreEvent, user, kernel), OffcorePmuRequest(name, offcoreEvent)]

  def buildRequests(self, event):
    """
    Builds pmu requests for the given event

    :param event: PMU event to be programmed

    """
    if event.uarchName not in self.eventsDb:
      raise Exception('failed locate pmu uarch event name {}'.format(event.uarchName))
    uarchEvent = self.eventsDb[event.uarchName]
    if uarchEvent.eventType not in self.eventTypeMap:
      raise Exception('pmu event {} is not currently supported'.format(event.uarchName))
    return self.eventTypeMap[uarchEvent.eventType](event.name, uarchEvent, event.user, event.kernel)

class RequestSorter(object):
  """Sorts pmu requests based on constraints"""

  @staticmethod
  def isSorted(requests):
    """
    Checks sort order for the given  list of pmu requests

    :param requests: A list of sorted or unsorted pmu requests

    """
    for i, request in enumerate(requests):
      if not request.event.canUse(i):
        return False
    return True

  @staticmethod
  def sort(requests):
    """
    Sorts a list of requests based on constraints

    :param requests:  A list of requests to be sorted

    """
    if RequestSorter.isSorted(requests):
      return requests
    indexSets = [req.event.validPmc for req in requests]
    allocator = Allocator(indexSets)
    if allocator.slotCount() == len(indexSets):
      allocation = allocator.allocate()
      if allocation:
        sortedRequests = [None] * len(requests)
        for i, req in enumerate(requests):
          sortedRequests[allocation[i]] = req
        return sortedRequests
    return None

  @staticmethod
  def reportConstraints(requests):
    """Builds a report of register allocation constraints for a list of requests"""
    report = ''
    for request in requests:
      report += '\n{:40s} -> {}'.format(request.event.name, request.event.validPmc)
    return report
