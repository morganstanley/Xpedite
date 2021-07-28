#!/usr/bin/env python
#################################################################################
##
## Xpedite auto generated file
##
#################################################################################

from xpedite import Probe, TxnBeginProbe, TxnSuspendProbe, TxnResumeProbe, TxnEndProbe
from xpedite.txn.classifier import ProbeDataClassifier
from xpedite.types import RouteConflation
from xpedite import TopdownNode, Metric, Event, ResultOrder

# Name of the application
appName = 'MyApp'

# Host, where the applciation is running
appHost = '10.162.217.143'

# Path of the appinfo file, configured in the application while initializing xpedite framework
appInfo = '/var/tmp/xpedite-appinfo.txt'

################################################## Probe List ##################################################
# Probes when enabled collect samples druing execution. The probes listed here are enabled during "xpedite record"
# Probes with types TxnBeginProbe and TxnEndProbe mark the beginning and end of transactions respectively. 
probes = [
  TxnBeginProbe ('Handler begin',   sysName = 'handlerBegin'),    # Begin of event Handler
  Probe         ('Parse Begin',     sysName = 'parseBegin'),      # Begin of message parsing
  Probe         ('Msg Alloc Begin', sysName = 'MsgAllocBegin'),   # Begin of message allocation
  Probe         ('Msg Alloc End',   sysName = 'MsgAllocEnd'),     # End of message allocation
  Probe         ('Tx Begin',        sysName = 'TxBegin'),         # Begin of transmission
  Probe         ('Tx end',          sysName = 'TxEnd'),           # End of transmission
  TxnEndProbe   ('handler end',     sysName = 'handlerEnd'),      # End of event Handler
]

############################################# Performance Counters #############################################
# List of performance counters to be collected for this profile
# The element can be one of 
#   1. Node in a topdown hierarchy
#   2. Metrics like IPC etc.
#   3. Raw hardware perforance counter for a micro architectural event
# To see topdown hierarchy run        - "xpedite topdown"
# To see available metrics run        - "xpedite metrics"
# To see available raw counters run   - "xpedite list"
#pmc = [
#  TopdownNode('Root'),         # top down analysis for Root node of the hierarchy
#  Metric('IPC'),               # computer instructions retired per cycle mertric
#  Event('kernel cycles',  'CPL_CYCLES.RING0'),
#  Event('Inst retired',   'INST_RETIRED.ANY_P'),
#  Event('Icache L1 miss', 'ICACHE.MISSES'),
#  Event('Icache L2 Hit',  'L2_RQSTS.CODE_RD_HIT'),
#  Event('Icache L2 miss', 'L2_RQSTS.CODE_RD_MISS'),
#  Event('Data L1 Miss',   'L2_RQSTS.DEMAND_DATA_RD_HIT'),
#  Event('Data L2 Miss',   'L2_RQSTS.DEMAND_DATA_RD_MISS'),
#  Event('LLC Miss',       'LONGEST_LAT_CACHE.MISS'),
#]
# List of cpu, where the harware performance counters will be enabled
#cpuSet = [8]


############################################# Benchmark transactions ############################################
# List of stored reports from previous runs, to be used for benchmarking
# you can create, new benchmarks using --createBenchmark option to record or report sub-commands
#benchmarkPaths = [ 
#  '/var/tmp/benchmarks/baseline',
#]


############################################## Filter transactions ##############################################
# filter transactions prior to report generation
# For instance, to limit the report generation to first 1000 transactions
# txnCount = 0
# def filter(profileName, txn):
#  global txnCount
#  if txnCount < 1000:
#   txnCount +=1
#   return True
# txnFilter = filter


############################################# Classify transactions #############################################
# classifiers are used to classify transaction into different types
# The Latency statistics and distribution are reported independently for each category of transactions
# For instance, an application might classify transactions based on the presence or absence of specific probe(s)
#class Classifier(object):
#  def classify(self, txn):
#    if txn.hasProbe(Probe('Tx Begin', sysName = 'TxBegin')):
#      return 'RoutedMessages'
#    return 'DroppedMessages'


############################################### Sort transactions ##############################################
# sort order of transaction metrics in report
# valid values
#  ResultOrder.Chronological    sort by choronological order, in which the transactions were seen by app
#  ResultOrder.WorstToBest      sort by statistics order, with worst to best in descending order
#  ResultOrder.BestToWorst      sort by statistics order, with best to worst in ascending order
#  ResultOrder.Transaction      sort by transaction sequence number
#resultOrder = ResultOrder.Chronological

############################################### Route conflation ##############################################
# conflate longer routes to shorter ones while bechmarking
# valid values
#  RouteConflation.On    sort by choronological order, in which the transactions were seen by app
#  RouteConflation.Off      sort by statistics order, with worst to best in descending order
#routeConflation = RouteConflation.On

################################################ Persist Report ################################################
# Home directory to store xpedite reports - xpedite record and report commands will persiste reports to home dir
# To open previously generated reports use - xpedite shell -H '<path to home dir>'
# homeDir = '~'
