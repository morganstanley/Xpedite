#!/usr/bin/env python
"""
Module to interact with Xpedite device driver to program H/W performance counters

This module provides logic to
  1. Detect and interact with Xpedite device driver
  2. Build and serialize requests from a list of pmc events

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import six
import struct
import logging
from xpedite.pmu.request     import (
                               PmuRequestFactory, GenericPmuRequest, OffcorePmuRequest,
                               FixedPmuRequest, RequestSorter
                             )
from xpedite.pmu.event       import EventSet

LOGGER = logging.getLogger(__name__)

XPEDITE_DEVICE = '/dev/xpedite'

def isDriverLoaded():
  """Checks status of Xpedite device driver"""
  return os.path.exists(XPEDITE_DEVICE) and os.access(XPEDITE_DEVICE, os.W_OK)

class PMUCtrl(object):
  """Interface to program pmu events with Xpedite device driver"""

  def __init__(self, eventsDb):
    self.device = None
    self.eventsDb = eventsDb

  def __enter__(self):
    if not isDriverLoaded():
      import socket
      hostname = socket.gethostname()
      raise Exception('Xpedite device driver not loaded | run "xpedite pmc --enable" at '
          'host {} to enable pmc'.format(hostname))
    self.device = open(XPEDITE_DEVICE, 'w')
    return self

  def __exit__(self, *args):
    if self.device:
      self.device.close()

  @staticmethod
  def buildRequestGroup(cpu, eventSet):
    """
    Builds a group of fixed, generic and offcore requests

    :param cpu: Id of the target cpu core
    :param eventSet: Collection of pmu request to be processed

    """
    request = struct.pack(
      '=BBBB', cpu, len(eventSet.fixedRequests), len(eventSet.genericRequests), len(eventSet.offcoreRequests)
    )
    for event in eventSet.fixedRequests:
      request += event.buildMask()
    for _ in range(len(eventSet.fixedRequests), 3):
      request += FixedPmuRequest.defaultMask()

    for event in eventSet.genericRequests:
      request += event.buildMask()
    for _ in range(len(eventSet.genericRequests), 8):
      request += GenericPmuRequest.defaultMask()

    for event in eventSet.offcoreRequests:
      request += event.buildMask()
    for _ in range(len(eventSet.offcoreRequests), 2):
      request += OffcorePmuRequest.defaultMask()
    return request

  @staticmethod
  def resolveEvents(eventsDb, cpuSet, events):
    """
    Resolves and build pmu requests for a list of events

    :param eventsDb: Handle to database of PMU events for the target cpu
    :param cpuSet: A set of cpu cores to enable pmu
    :param events: A list of pmu events to be resolved

    """
    if len(events) > 11:
      raise Exception('cannot enable more than 11 events - requested {}'.format(len(events)))

    requestFactory = PmuRequestFactory(eventsDb)
    eventSet = EventSet(cpuSet)
    for event in events:
      requests = requestFactory.buildRequests(event)
      for request in requests:
        if isinstance(request, GenericPmuRequest):
          eventSet.addGenericPmuRequest(request)
        elif isinstance(request, OffcorePmuRequest):
          eventSet.addOffcorePmuRequest(request)
        elif isinstance(request, FixedPmuRequest):
          eventSet.addFixedPmuRequest(request)
        else:
          raise Exception('detected invalid event type {} in pmu request'.format(type(event)))
    return eventSet

  @staticmethod
  def allocateEvents(eventSet):
    """
    Allocates registers for pmu events, while obeying constraints

    :param eventSet: A collection of resolved pmu events

    """
    if eventSet.genericRequests:
      sortedRequests = RequestSorter.sort(eventSet.genericRequests)
      if sortedRequests and len(sortedRequests) == len(eventSet.genericRequests):
        eventSet.genericRequests = sortedRequests
      else:
        pmcStr = '\n\t\t'.join((str(request) for request in eventSet.genericRequests))
        report = RequestSorter.reportConstraints(eventSet.genericRequests)
        errMsg = """Failed to program selected counters
          --> chosen pmc - \n\t\t{}
          --> reordered pmc - {}
          The following constraints prevent all selected counter from being used simultaneously
          {}""".format(pmcStr, sortedRequests, report)
        raise Exception(errMsg)

  @staticmethod
  def buildEventSet(eventsDb, cpuSet, events):
    """
    Resolves a list of events to a set of programmable pmu event select values

    :param eventsDb: Handle to database of PMU events for the target cpu
    :param cpuSet: A set of cpu cores to enable pmu
    :param events: A list of pmu events to be enabled

    """
    eventSet = PMUCtrl.resolveEvents(eventsDb, cpuSet, events)
    PMUCtrl.allocateEvents(eventSet)
    return eventSet

  def enable(self, cpuSet, events):
    """
    Enables pmu events in a set of target cpus

    :param cpuSet: A set of cpu cores to enable pmu
    :param events: A list of pmu events to be enabled

    """
    if not self.device:
      raise Exception('xpedite device not enabled - use "with PMUCtrl() as pmuCtrl:" to init device')

    eventSet = self.buildEventSet(self.eventsDb, cpuSet, events)
    for cpu in cpuSet:
      requestGroup = self.buildRequestGroup(cpu, eventSet)
      LOGGER.debug(
        'sending request (%d bytes) to xpedite ko [%s]',
        len(requestGroup), ':'.join('{:02x}'.format(six.indexbytes(requestGroup, i))
                                    for i in range(0, len(requestGroup)))
      )
      self.device.write(requestGroup)
      self.device.flush()
    return eventSet

  @staticmethod
  def buildPerfEventsRequest(eventsDb, events):
    """
    Builds a request to enable events with perf events api

    :param eventsDb: Handle to database of PMU events for the target cpu
    :param events: A list of pmu events to be enabled

    """
    eventSet = PMUCtrl.buildEventSet(eventsDb, [], events)
    if not eventSet.offcoreRequestCount():
      requestGroup = PMUCtrl.buildRequestGroup(0, eventSet)
      pdu = ':'.join('{:02x}'.format(six.indexbytes(requestGroup, i)) for i in range(0, len(requestGroup)))
      return (eventSet, pdu)
    return (None, None)
