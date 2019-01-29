"""
This package contains pytests for Xpedite's performance counters, including:

- Tests for allocation
- A test to orchestrate events loading
- Test for PMC related commands: metrics, events, and topdown

Below is a lit of CPUs currently supported by Xpedite
"""

PMU_DATA = 'pmuData'
EVENTS_DB_FILE_NAME = 'events.txt'
METRICS_FILE_NAME = 'metrics.txt'
TOPDOWN_FILE_NAME = 'topdown.txt'

CPU_IDS = [
  'GenuineIntel-6-2A', 'GenuineIntel-6-2D', 'GenuineIntel-6-3A', 'GenuineIntel-6-3E',
  'GenuineIntel-6-3C', 'GenuineIntel-6-45', 'GenuineIntel-6-46', 'GenuineIntel-6-3D',
  'GenuineIntel-6-47', 'GenuineIntel-6-4F', 'GenuineIntel-6-4E', 'GenuineIntel-6-5E',
  'GenuineIntel-6-8E', 'GenuineIntel-6-9E', 'GenuineIntel-6-3F',
]
