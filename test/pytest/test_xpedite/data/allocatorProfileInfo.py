#!/usr/bin/env python2.7
#################################################################################
##
## Xpedite auto generated file
##
#################################################################################

from xpedite import Probe, TxnBeginProbe, TxnEndProbe

# Name of the application
appName = 'allocatorApp'

# Host, where the applciation is running
appHost = '127.0.0.1'

# Path of the appinfo file, configured in the application while initializing xpedite framework
appInfo = 'xpedite-appinfo.txt'

################################################## Probe List ##################################################
# Probes when enabled collect samples druing execution. The probes listed here are enabled during "xpedite record"
# Probes with types TxnBeginProbe and TxnEndProbe mark the beginning and end of transactions respectively. 
probes = [
  TxnBeginProbe('Allocation Begin', sysName = 'AllocationBegin'),
  TxnEndProbe('Allocation End', sysName = 'AllocationEnd'),
  Probe('Calloc Begin', sysName = 'CallocBegin'),
  Probe('Calloc End', sysName = 'CallocEnd'),
  Probe('Free Begin', sysName = 'FreeBegin'),
  Probe('Free End', sysName = 'FreeEnd'),
  Probe('Malloc Begin', sysName = 'MallocBegin'),
  Probe('Malloc End', sysName = 'MallocEnd'),
  Probe('Mmap Begin', sysName = 'MmapBegin'),
  Probe('Mmap End', sysName = 'MmapEnd'),
  Probe('Munmap Begin', sysName = 'MunmapBegin'),
  Probe('Munmap End', sysName = 'MunmapEnd'),
  Probe('New Begin', sysName = 'NewBegin'),
  Probe('New End', sysName = 'NewEnd'),
  Probe('Posix Memalign Begin', sysName = 'PosixMemalignBegin'),
  Probe('Posix Memalign End', sysName = 'PosixMemalignEnd'),
  Probe('Realloc Begin', sysName = 'ReallocBegin'),
  Probe('Realloc End', sysName = 'ReallocEnd'),
]
