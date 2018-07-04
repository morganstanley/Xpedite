#################################################################################
##
## Xpedite profile information file
##
#################################################################################

from xpedite import Probe, TxnBeginProbe, TxnEndProbe
from xpedite.pmu.event import Event
from xpedite.txn.classifier import ProbeDataClassifier
from xpedite import TopdownNode, Metric, Event, ResultOrder

# Name of the application
appName = 'xpediteDemo'

# Host, where the applciation is running
appHost = '127.0.0.1'

# Path of the appinfo file, configured in the application while initializing xpedite framework
appInfo = 'xpedite-appinfo.txt'

################################################## Probe List ##################################################
# Probes when enabled collect samples druing execution. The probes listed here are enabled during "xpedite record"
# Probes with types TxnBeginProbe and TxnEndProbe mark the beginning and end of transactions respectively. 
probes = [
  TxnBeginProbe('Work Begin', sysName = 'WorkBegin'),
  TxnEndProbe('Work End', sysName = 'WorkEnd'),
]

#List of cpu, where the harware performance counters will be enabled
cpuSet = [0]
