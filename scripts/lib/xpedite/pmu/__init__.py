"""
PMU Package is used to program and collect H/W performance counters

This package provides logic to
  1. Build a database of performance counters for supported micro architectures
  2. Interact with xpedite kernel module to programming performance counters
  3. Allocate pmc events to general pmc registers, obeying constraints
  4. Topdown hierarchy and metrics computations

Author: Manikandan Dhamodharan, Morgan Stanley
"""
from xpedite.dependencies import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Six)
