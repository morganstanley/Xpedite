#################################################################################
##
## Xpedite auto generated file
##
#################################################################################

import os
from xpedite import Probe, TxnBeginProbe, TxnSuspendProbe, TxnResumeProbe, TxnEndProbe
from xpedite import TopdownNode, Metric, Event

# Name of the application
appName = 'XpediteDemo'

# Host, where the applciation is running
appHost = os.environ.get('XPEDITE_DEMO_APP_HOST')

# Path of the appinfo file, configured in the application while initializing xpedite framework
appInfo = os.path.join(os.environ.get('XPEDITE_DEMO_LOG_DIR'), 'xpedite-appinfo.txt')

################################################## Probe List ##################################################
# Probes when enabled collect samples druing execution. The probes listed here are enabled during "xpedite record"
# Probes with types TxnBeginProbe and TxnEndProbe mark the beginning and end of transactions respectively. 
probes = [
  TxnBeginProbe('Work Begin', sysName = 'WorkBegin'),
  TxnEndProbe('Work End', sysName = 'WorkEnd'),
]

############################################# Performance Counters #############################################
# List of performance counters to be collected for this profile
# The element can be one of 
#   1. Node in a topdown hierarchy
#   2. Metrics like IPC etc.
#   3. Raw hardware perforance counter for a micro architectural event
# To see topdown hierarchy run        - "xpedite topdown"
# To see available metrics run        - "xpedite metrics"
# To see available raw counters run   - "xpedite evlist"
pmc = None
if os.environ.get('XPEDITE_DEMO_PMC'):
  pmc = [
    TopdownNode('Root'),          # top down analysis for Root node of the hierarchy
    Metric('IPC'),               # computer instructions retired per cycle mertric
    #Event('LLC Miss',            'LONGEST_LAT_CACHE.MISS'),
  ]

#List of cpu, where the harware performance counters will be enabled
cpuSet = [int(c) for c in os.environ.get('XPEDITE_DEMO_CPU_SET').split(',')]

