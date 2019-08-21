#!/usr/bin/env python
"""
Class definition to store attributes of micro architectural events

This module provides definitions for the following events
  1. generic core - events programmable in general purpose pmu registers
  2. fixed core   - events colleted by fixed pmu registers
  3. offcore      - counters for offcore requests and responses

Author: Manikandan Dhamodharan, Morgan Stanley
"""

class GenericCoreEvent(object):
  """Generic PMU Core Event"""

  eventType = 'GenericCore'
  def __init__(self):
    self.eventType = GenericCoreEvent.eventType
    self.name = None
    self.eventSelect = None
    self.unitMask = None
    self.counterMask = None
    self.invert = None
    self.briefDescription = None
    self.description = None
    self._validSmtPmc = None
    self._validPmc = None
    self.msrIndex = None
    self.msrValue = None
    self.anyThread = None
    self.edgeDetect = None
    self.pebs = None
    self.takenAlone = None
    self.dataLA = None
    self.l1HitIndication = None
    self.errata = None
    self.isOffCore = False

  @property
  def validPmc(self):
    """returns a set of programmable pmc registers"""
    return self._validPmc if self._validPmc else self._validSmtPmc

  def unInitialized(self):
    """Checks if all required attribues are initialized"""
    return (
      self.name is None
      or self.eventSelect is None
      or self.unitMask is None
      or self.counterMask is None
      or self.invert is None
      or self.briefDescription is None
      or self.description is None
      or self.validPmc is None
      or self.msrIndex is None
      or self.msrValue is None
      or self.anyThread is None
      or self.edgeDetect is None
      or self.pebs is None
    )

  def rawValue(self):
    """Returns a raw bit mask for this event"""
    val = self.eventSelect | (self.unitMask << 8)
    val |= self.edgeDetect << 18
    val |= self.anyThread << 21
    val |= self.counterMask << 24
    val |= self.invert << 23
    return val

  def canUse(self, counterIndex):
    """
    Checks if this pmu event can be programmed at given register index

    :param counterIndex: Index of the register to check

    """
    return counterIndex in self.validPmc # pylint: disable=unsupported-membership-test

  def isConstrained(self):
    """Checks if this event imposes, any constraints on programmable registers"""
    return len(self.validPmc) < 8


  def __repr__(self):
    """Returns string representation of this generic core event"""
    counterMaskRepr = ' [CMask-{}]'.format(self.counterMask) if self.counterMask else ''
    return '{:60s} [0x{:08X}]      - {}{}'.format(self.name, self.rawValue(), self.briefDescription, counterMaskRepr)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class FixedCoreEvent(GenericCoreEvent):
  """Fixed PMU Core Event"""

  eventType = 'FixedCore'
  def __init__(self):
    GenericCoreEvent.__init__(self)
    self.eventType = FixedCoreEvent.eventType

  def __repr__(self):
    eventName = '{} (FixedCtr {})'.format(self.name, self.validPmc)
    return '{:60s} [FixedCtr-{}]      - {}'.format(eventName, self.validPmc, self.briefDescription)

class OffCoreEvent(GenericCoreEvent):
  """
  Offcore events need to program two general purpose counter with with request and response event select codes
  in addition to two request/response msr registers. The event select in this case will be an array instead of a scalar
  value
  """

  eventType = 'OffCore'

  eventSelects = [int('B7', 16), int('BB', 16)]

  def __init__(self):
    GenericCoreEvent.__init__(self)
    self.eventType = OffCoreEvent.eventType
    self.isOffCore = True

  def __repr__(self):
    """str representation of a PMUGpEvent"""
    return '{:60s} [0x{:02X},0x{:02X}|0x{:02X}] - {}'.format(
      self.name,
      self.eventSelect[0], self.eventSelect[1] if len(self.eventSelect) > 1 else 0,  # pylint: disable=unsubscriptable-object
      self.unitMask, self.briefDescription
    )

class UnCoreEvent(object):
  """Offcore PMU Core Event"""

  eventType = 'UnCore'
  def __init__(self):
    self.eventType = UnCoreEvent.eventType
