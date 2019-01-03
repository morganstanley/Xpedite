"""

Test to orchestrate events loading

Author: Manikandan Dhamodharan, Morgan Stanley

"""

import os
from xpedite.pmu.pmuctrl      import PMUCtrl
from xpedite.pmu.eventsLoader import EventsLoader
from xpedite.pmu.event        import Event
from logger                   import LOG_CONFIG_PATH
import logging
import logging.config

logging.config.fileConfig(LOG_CONFIG_PATH)
LOGGER = logging.getLogger('xpedite')

def test_request_sorter():
  """
  Test loading and sorting of performance counter events
  """
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
  LOGGER.info(eventState.genericRequests)
  for i, event in enumerate(events):
    assert event.uarchName == eventState.genericRequests[len(events)-i-1].uarchName
