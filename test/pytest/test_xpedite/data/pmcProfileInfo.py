#################################################################################
##
## Xpedite profile information file
##
#################################################################################

import os
from test_xpedite.data.profileInfo import *

############################################# Performance Counters #############################################
# List of performance counters to be collected for this profile
# The element can be one of 
#   1. Node in a topdown hierarchy
#   2. Metrics like IPC etc.
#   3. Raw hardware perforance counter for a micro architectural event
# To see topdown hierarchy run        - "xpedite topdown"
# To see available metrics run        - "xpedite metrics"
# To see available raw counters run   - "xpedite evlist"
pmc = [
  TopdownNode('Root'),          # top down analysis for Root node of the hierarchy
  Metric('IPC'),                # computer instructions retired per cycle mertric
  Event('kernel cycles',        'CPL_CYCLES.RING0'),
  Event('LLC Miss',             'LONGEST_LAT_CACHE.MISS'),
]

#List of cpu, where the harware performance counters will be enabled
cpuSet = [0]
