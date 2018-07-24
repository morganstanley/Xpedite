"""

Test to orchestrate register allocation and events loading

Author: Manikandan Dhamodharan, Morgan Stanley

"""

import os
import pytest
from xpedite.pmu.pmuctrl import PMUCtrl
from xpedite.pmu.eventsLoader import EventsLoader
from xpedite.pmu.eventsDb import loadEventsDb
from xpedite.pmu.request import RequestSorter
from xpedite.pmu.event import Event

def test_request_sorter():
  eventsFile = os.path.join(os.path.dirname(__file__), 'test_events.json')
  eventsDb = EventsLoader().loadJson(eventsFile)
  assert eventsDb
  events = [
    Event('EVENT_3', 'EVENT_3'),
    Event('EVENT_2', 'EVENT_2'),
    Event('EVENT_1', 'EVENT_1'),
    Event('EVENT_0', 'EVENT_0'),
  ]
  eventState = PMUCtrl.resolveEvents(eventsDb, [0], events)
  assert len(eventState) == len(events)
  assert len(eventState.genericRequests) == len(events)
  for i, event in enumerate(events):
    assert event.uarchName == eventState.genericRequests[i].uarchName
  PMUCtrl.allocateEvents(eventState)
  assert len(eventState.genericRequests) == len(events)
  print(eventState.genericRequests)
  for i, event in enumerate(events):
    assert event.uarchName == eventState.genericRequests[len(events)-i-1].uarchName
