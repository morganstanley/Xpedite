#!/usr/bin/env python2.7
"""
Module to interact with Xpedite device driver to program H/W performance counters

This module provides logic to
  1. Detect and interact with Xpedite device driver
  2. Build and serialize requests from a list of pmc events

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import struct
import logging
from xpedite.pmu.request     import (
                               PmuRequestFactory, GenericPmuRequest, OffcorePmuRequest,
                               FixedPmuRequest, RequestSorter
                             )
from xpedite.pmu.event       import EventState

LOGGER = logging.getLogger(__name__)

PMU_CTRL_DEVICE = '/dev/xpedite'

def canUsePMC():
  """Checks status of Xpedite device driver"""
  return os.path.exists(PMU_CTRL_DEVICE) and os.access(PMU_CTRL_DEVICE, os.W_OK)

class PMUCtrl(object):
  """Interface to program pmu events with Xpedite device driver"""

  def __init__(self, eventsDb):
    self.device = None
    self.eventsDb = eventsDb

  def __enter__(self):
    if not canUsePMC():
      import socket
      hostname = socket.gethostname()
      raise Exception('PMC not enabled - run "xpedite pmc --enable" to load kernel module at host {}'.format(hostname))
    self.device = open(PMU_CTRL_DEVICE, 'w')
    return self

  def __exit__(self, *args):
    if self.device:
      self.device.close()

  @staticmethod
  def buildRequestGroup(cpu, eventState):
    """
    Builds a group of fixed, generic and offcore requests

    :param cpu: Id of the target cpu core
    :param eventState: Collection of pmu request to be processed

    """
    request = struct.pack(
      '=BBBB', cpu, len(eventState.fixedRequests), len(eventState.genericRequests), len(eventState.offcoreRequests)
    )
    for event in eventState.fixedRequests:
      request += event.buildMask()
    for _ in range(len(eventState.fixedRequests), 3):
      request += FixedPmuRequest.defaultMask()

    for event in eventState.genericRequests:
      request += event.buildMask()
    for _ in range(len(eventState.genericRequests), 8):
      request += GenericPmuRequest.defaultMask()

    for event in eventState.offcoreRequests:
      request += event.buildMask()
    for _ in range(len(eventState.offcoreRequests), 2):
      request += OffcorePmuRequest.defaultMask()
    return request

  def resolveEvents(self, cpuSet, events):
    """
    Resolves and build pmu requests for a list of events

    :param cpuSet: A set of cpu cores to enable pmu
    :param events: A list of pmu events to be resolved

    """
    if len(events) > 11:
      raise Exception('PMUCtrl - cannot enable more than 11 events - requested {}'.format(len(events)))

    requestFactory = PmuRequestFactory(self.eventsDb)
    eventState = EventState(cpuSet)
    for event in events:
      requests = requestFactory.buildRequests(event)
      for request in requests:
        if isinstance(request, GenericPmuRequest):
          eventState.addGenericPmuRequest(request)
        elif isinstance(request, OffcorePmuRequest):
          eventState.addOffcorePmuRequest(request)
        elif isinstance(request, FixedPmuRequest):
          eventState.addFixedPmuRequest(request)
        else:
          raise Exception('PMUCtrl request - invalid event type {}'.format(type(event)))
    return eventState

  @staticmethod
  def allocateEvents(eventState):
    """
    Allocates registers for pmu events, while obeying constraints

    :param eventState: A collection of resolved pmu events

    """
    if eventState.genericRequests:
      sortedRequests = RequestSorter.sort(eventState.genericRequests)
      if sortedRequests and len(sortedRequests) == len(eventState.genericRequests):
        eventState.genericRequests = sortedRequests
      else:
        pmcStr = '\n\t\t'.join((str(request) for request in eventState.genericRequests))
        report = RequestSorter.reportConstraints(eventState.genericRequests)
        errMsg = """Failed to program selected counters
          --> chosen pmc - \n\t\t{}
          --> reordered pmc - {}
          The following constraints prevent all selected counter from being used simultaneously
          {}""".format(pmcStr, sortedRequests, report)
        raise Exception(errMsg)

  def enable(self, cpuSet, events):
    """
    Enables pmu events in a set of target cpus

    :param cpuSet: A set of cpu cores to enable pmu
    :param events: A list of pmu events to be enabled

    """
    if not self.device:
      raise Exception('PMUCtrl - xpedite device not enabled - use "with PMUCtrl() as pmuCtrl:" to init device')

    eventState = self.resolveEvents(cpuSet, events)
    self.allocateEvents(eventState)
    for cpu in cpuSet:
      requestGroup = self.buildRequestGroup(cpu, eventState)
      LOGGER.debug(
        'sending request (%d bytes) to xpedite ko [%s]',
        len(requestGroup), ':'.join('{:02x}'.format(ord(request)) for request in requestGroup)
      )
      self.device.write(requestGroup)
      self.device.flush()
    return eventState
