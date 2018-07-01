"""
Module to allocate general purpose pmc event to available registers

General purpose pmc events may have constraints on register allocation.
Given a set of pmc events, this module attempts to allocate events optimally
to satisfy constraints for all events in the set.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import copy

class Allocatable(object):
  """Stores constraints and state of a register allocatable event"""

  def __init__(self, index, constraint):
    self.index = index
    self.constraint = copy.deepcopy(constraint)

  def __repr__(self):
    return 'allocatable [{}] -> {}'.format(self.index, self.constraint)

class Allocator(object):
  """Allocates PMU events to a set of available registers, while obeying constraints"""

  def __init__(self, constraints):
    self.constraints = constraints
    self.allocatables = [Allocatable(i, constraint) for i, constraint in enumerate(constraints)]
    maxIndex = max([max(constraint) for constraint in constraints])
    self.slots = [-1] * (maxIndex + 1)
    self.allocation = [-1] * len(constraints)

  def slotCount(self):
    """Returns the number of slots needed for allocation"""
    return len(self.slots)

  def allocate(self):
    """Allocates PMU events to a set of available registers, while obeying constraints"""
    self.allocatables.sort(key=lambda a: len(a.constraint), reverse=True)
    allocatable = self.allocatables.pop()
    for cn in allocatable.constraint:
      if self.slots[cn] == -1:
        self.slots[cn] = allocatable.index
        self.allocation[allocatable.index] = cn
        if self.allocatables:
          for allocatable in self.allocatables:
            allocatable.constraint.discard(cn)
            return self.allocate()
        return self.allocation
    return None

  def report(self):
    """Returns a report of allocations"""
    report = ''
    for i, constraint in enumerate(self.constraints):
      report += '\n\t constraint {} -> {}'.format(constraint, self.allocation[i])
    return report
