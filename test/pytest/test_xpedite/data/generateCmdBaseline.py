#!/usr/bin/env python2.7
#################################################################################
##
## Xpedite auto generated file
##
#################################################################################

from xpedite import Probe, TxnBeginProbe, TxnEndProbe
from xpedite.pmu.event import Event
from xpedite.txn.classifier import ProbeDataClassifier
from xpedite import TopdownNode, Metric, Event, ResultOrder

# Name of the application
appName = 'slowFixDecoder'

# Host, where the applciation is running
appHost = '127.0.0.1'

# Path of the appinfo file, configured in the application while initializing xpedite framework
appInfo = 'xpedite-appinfo.txt'

################################################## Probe List ##################################################
# Probes when enabled collect samples druing execution. The probes listed here are enabled during "xpedite record"
# Probes with types TxnBeginProbe and TxnEndProbe mark the beginning and end of transactions respectively. 
probes = [
  Probe('Parse Account', sysName = 'ParseAccount'),
  Probe('Parse Begin Msg', sysName = 'ParseBeginMsg'),
  Probe('Parse Body Length', sysName = 'ParseBodyLength'),
  Probe('Parse Cl Order Id', sysName = 'ParseClOrderId'),
  TxnBeginProbe('Parse Fix Begin', sysName = 'ParseFixBegin'),
  TxnEndProbe('Parse Fix End', sysName = 'ParseFixEnd'),
  Probe('Parse Handler Inst', sysName = 'ParseHandlerInst'),
  Probe('Parse Host', sysName = 'ParseHost'),
  Probe('Parse Message Seq Num', sysName = 'ParseMessageSeqNum'),
  Probe('Parse Message Type', sysName = 'ParseMessageType'),
  Probe('Parse Order Type', sysName = 'ParseOrderType'),
  Probe('Parse Price', sysName = 'ParsePrice'),
  Probe('Parse Qty', sysName = 'ParseQty'),
  Probe('Parse Sender Comp Id', sysName = 'ParseSenderCompId'),
  Probe('Parse Sending Time', sysName = 'ParseSendingTime'),
  Probe('Parse Side', sysName = 'ParseSide'),
  Probe('Parse Symbol', sysName = 'ParseSymbol'),
  Probe('Parse Time In Force', sysName = 'ParseTimeInForce'),
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
cpuSet = [0]


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
#sort order of transaction metrics in report
# valid values
#  ResultOrder.Chronological    sort by choronological order, in which the transactions were seen by app
#  ResultOrder.WorstToBest      sort by statistics order, with worst to best in descending order
#  ResultOrder.BestToWorst      sort by statistics order, with best to worst in ascending order
#  ResultOrder.Transaction      sort by transaction sequence number
#resultOrder = ResultOrder.Chronological


################################################ Persist Report ################################################
# Home directory to store xpedite reports - xpedite record and report commands will persiste reports to home dir
# To open previously generated reports use - xpedite shell -H '<path to home dir>'
# homeDir = '~'
