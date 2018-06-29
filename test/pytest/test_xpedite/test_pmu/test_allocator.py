"""

Test to exercise register allocation of pmc events

PMC events can have constraints on general purpose registers that could be 
used for collection of performance counters.

This test ensures the register allocation works while obeying 
the constraints for a set of pmc events

Author: Manikandan Dhamodharan, Morgan Stanley

"""

import os
import pytest
from xpedite.pmu.pmuctrl import PMUCtrl
from xpedite.pmu.eventsLoader import EventsLoader
from xpedite.pmu.eventsDb import loadEventsDb
from xpedite.pmu.request import RequestSorter
from xpedite.pmu.event import Event
from xpedite.pmu.allocator import Allocator

class TestAllocator(object):

  def validate_allocation(self, indexSets, allocation):
    assert allocation
    assert len(allocation) == len(indexSets)
    assert len(allocation) == len(set(allocation)) 
    for i, indexSet in enumerate(indexSets):
      assert allocation[i] in indexSet 

  def test_allocator_single_sets(self):
    indexSets = [ set([3]), set([2]), set([1]), set([0])]
    allocator = Allocator(indexSets)
    assert len(allocator.slots) == 4
    allocation = allocator.allocate()
    print(allocator.report())
    self.validate_allocation(indexSets, allocation)

  def test_allocator_double_sets(self):
    indexSets = [ set([3, 1]), set([2, 1]), set([0, 1]), set([1])]
    allocator = Allocator(indexSets)
    assert len(allocator.slots) == 4
    allocation = allocator.allocate()
    print(allocator.report())
    self.validate_allocation(indexSets, allocation)

  def test_allocator_triple_sets(self):
    indexSets = [ set([3, 1, 4]), set([3, 2, 1]), set([1]), set([0, 1])]
    allocator = Allocator(indexSets)
    assert len(allocator.slots) == 5
    allocation = allocator.allocate()
    print(allocator.report())
    self.validate_allocation(indexSets, allocation)

  def test_allocator_quad_sets(self):
    indexSets = [ set([4]), set([3, 2]), set([1, 2]), set([1, 2, 3, 4])]
    allocator = Allocator(indexSets)
    assert len(allocator.slots) == 5
    allocation = allocator.allocate()
    print(allocator.report())
    self.validate_allocation(indexSets, allocation)

  def test_allocator_octa_sets(self):
    indexSets = [ set([0, 1,2,3]), set([0, 1,2,3]), set([0, 1,2,3]), set([0,1,2,3,4, 5, 6, 7]) ]
    allocator = Allocator(indexSets)
    assert len(allocator.slots) == 8
    allocation = allocator.allocate()
    print(allocator.report())
    self.validate_allocation(indexSets, allocation)

  def test_allocator_single_confilicting_sets(self):
    indexSets = [ set([1]), set([1])]
    allocator = Allocator(indexSets)
    assert len(allocator.slots) == 2
    allocation = allocator.allocate()
    assert not allocation

def request_sorter(): #test_
  eventsFile = os.path.join(os.path.dirname(__file__), 'test_events.json')
  eventsDb = EventsLoader().loadJson(eventsFile)
  assert eventsDb
  pmuctrl = PMUCtrl(eventsDb)
  events = [
    Event('EVENT_3', 'EVENT_3'),
    Event('EVENT_2', 'EVENT_2'),
    Event('EVENT_1', 'EVENT_1'),
    Event('EVENT_0', 'EVENT_0'),
  ]
  eventState = pmuctrl.resolveEvents([0], events)
  assert len(eventState) == len(events)
  assert len(eventState.genericRequests) == len(events)
  for i, event in enumerate(events):
    assert event.uarchName == eventState.genericRequests[i].uarchName
  pmuctrl.allocateEvents(eventState)
  assert len(eventState.genericRequests) == len(events)
  print(eventState.genericRequests)
  for i, event in enumerate(events):
    assert event.uarchName == eventState.genericRequests[len(events)-i-1].uarchName
