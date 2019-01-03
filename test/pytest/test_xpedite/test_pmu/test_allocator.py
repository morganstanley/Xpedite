"""

Test to exercise register allocation of pmc events

PMC events can have constraints on general purpose registers that could be
used for collection of performance counters.

This test ensures the register allocation works while obeying
the constraints for a set of pmc events

Author: Manikandan Dhamodharan, Morgan Stanley

"""

from xpedite.pmu.allocator    import Allocator
from logger                   import LOG_CONFIG_PATH
import logging
import logging.config

logging.config.fileConfig(LOG_CONFIG_PATH)
LOGGER = logging.getLogger('xpedite')

def validate_allocation(indexSets, allocation):
  """
  Utility function to validate allocation
  """
  assert allocation
  assert len(allocation) == len(indexSets)
  assert len(allocation) == len(set(allocation))
  for i, indexSet in enumerate(indexSets):
    assert allocation[i] in indexSet

def test_allocator_single_sets():
  """
  Test allocation with single sets
  """
  indexSets = [set([3]), set([2]), set([1]), set([0])]
  allocator = Allocator(indexSets)
  assert len(allocator.slots) == 4
  allocation = allocator.allocate()
  LOGGER.info(allocator.report())
  validate_allocation(indexSets, allocation)

def test_allocator_double_sets():
  """
  Test allocation with double sets
  """
  indexSets = [set([3, 1]), set([2, 1]), set([0, 1]), set([1])]
  allocator = Allocator(indexSets)
  assert len(allocator.slots) == 4
  allocation = allocator.allocate()
  LOGGER.info(allocator.report())
  validate_allocation(indexSets, allocation)

def test_allocator_triple_sets():
  """
  Test allocation with triple sets
  """
  indexSets = [set([3, 1, 4]), set([3, 2, 1]), set([1]), set([0, 1])]
  allocator = Allocator(indexSets)
  assert len(allocator.slots) == 5
  allocation = allocator.allocate()
  LOGGER.info(allocator.report())
  validate_allocation(indexSets, allocation)

def test_allocator_quad_sets():
  """
  Test allocation with quadruple sets
  """
  indexSets = [set([4]), set([3, 2]), set([1, 2]), set([1, 2, 3, 4])]
  allocator = Allocator(indexSets)
  assert len(allocator.slots) == 5
  allocation = allocator.allocate()
  LOGGER.info(allocator.report())
  validate_allocation(indexSets, allocation)

def test_allocator_octa_sets():
  """
  Test allocation with sets of 8
  """
  indexSets = [set([0, 1, 2, 3]), set([0, 1, 2, 3]), set([0, 1, 2, 3]), set([0, 1, 2, 3, 4, 5, 6, 7])]
  allocator = Allocator(indexSets)
  assert len(allocator.slots) == 8
  allocation = allocator.allocate()
  LOGGER.info(allocator.report())
  validate_allocation(indexSets, allocation)

def test_allocator_single_confilicting_sets():
  """
  Test allocation with matching sets
  """
  indexSets = [set([1]), set([1])]
  allocator = Allocator(indexSets)
  assert len(allocator.slots) == 2
  allocation = allocator.allocate()
  assert not allocation
