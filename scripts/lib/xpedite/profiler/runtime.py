"""
This module implements runtime logic to orchestrate the following steps
  1. Attach to a target application
  2. Resolve and activate probes
  3. Resolve and program pmc events
  4. Collect profile data for reporting

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import logging
from xpedite.txn.classifier       import DefaultClassifier
from xpedite.types            import ResultOrder
import time

LOGGER = logging.getLogger(__name__)

class AbstractRuntime(object):
  """
  Base class for xpedite runtime to orchestrate a profile session.

  The Abstract runtime provides implemetation to resolve/enable probes and pmu events

  """

  def __init__(self, app, probes):
    """
    Creates a new instance of Abstract runtime

    The constructor also instantiates caches for pmu events and topdown metrics

    :param app: an instance of xpedite app, to interact with target application
    :type app: xpedite.profiler.app.XpediteApp
    :param probes: List of probes to be enabled for the current profile session
    """

    from xpedite.profiler.resolver import ProbeResolver
    from xpedite.pmu.eventsDb import EventsDbCache
    from xpedite.pmu.topdown import TopdownCache
    from xpedite.profiler.app import pingApp
    pingApp(app)
    self.app = app
    self.probes = probes
    self.probeResolver = ProbeResolver()
    self.cpuInfo = None
    self.eventsDbCache = EventsDbCache()
    self.topdownCache = TopdownCache(self.eventsDbCache)
    self.topdownMetrics = None
    self.eventSet = None

  @staticmethod
  def formatProbes(probes):
    """
    Formats a list of probes to string

    :param probes: List of probes to be enabled for the current profile session

    """
    probeStr = ''
    for probe in probes:
      probeStr = probeStr + '\n\t{}'.format(probe)
    return probeStr

  def enableProbes(self, probes):
    """
    Enables the list of given probes

    :param probes: List of probes to be enabled for the current profile session

    """
    from xpedite.profiler.probeAdmin import ProbeAdmin
    LOGGER.debug('Enabling probes %s', self.formatProbes(probes))
    if probes:
      if self.eventSet:
        errMsg = ProbeAdmin.enablePMU(self.app, self.eventSet)
        if errMsg:
          msg = 'failed to enable PMU ({})'.format(errMsg)
          LOGGER.error(msg)
          raise Exception(msg)
      (errCount, errMsg) = ProbeAdmin.updateProbes(self.app, probes, targetState=True)
      if errCount > 0:
        msg = 'failed to enable probes ({} error(s))\n{}'.format(errCount, errMsg)
        LOGGER.error(msg)
        raise Exception(msg)
    else:
      LOGGER.warn('failed to enable probes - Invalid or empty probes argument')

  def resolveProbes(self, probes):
    """
    Checks the validity of the list of given probes

    :param probes: List of probes to be enabled for the current profile session

    """

    from xpedite.types.probe  import AnchoredProbe
    anchoredProbes = []
    LOGGER.debug('Resolving probes %s', self.formatProbes(probes))
    for probe in probes:
      if not probe.isAnchored():
        resolvedProbes = self.probeResolver.resolveAnchoredProbe(self.app, probe)
        if resolvedProbes:
          for rp in resolvedProbes:
            anchoredProbe = AnchoredProbe(
              probe.name, filePath=rp.filePath, lineNo=rp.lineNo, attributes=rp.attributes,
                isActive=rp.isActive, sysName=rp.sysName
            )
            anchoredProbes.append(anchoredProbe)
            LOGGER.debug('Resolved probe %s to anchored probe %s', probe, anchoredProbe)
        else:
          raise Exception('probe {} cannot be located in app. Please check if it\'s a valid probe'.format(
            probe.sysName
          ))
      else:
        anchoredProbes.append(probe)
    return anchoredProbes

  @staticmethod
  def resolveEvents(eventsDb, cpuSet, events):
    """
    Resolves a list of given pmu events from events database

    :param eventsDb: Handle to database of PMU events for the target cpu
    :param events: List of PMU events to be enabled for the current profile session
    :param cpuSet: List of cpu, where the userspace pmu collection will be enabled

    """
    from xpedite.pmu.pmuctrl import PMUCtrl
    return PMUCtrl.resolveEvents(eventsDb, cpuSet, events)

  @staticmethod
  def aggregatePmc(pmc):
    """
    Aggreagtes given pmu events to create a unique list of events

    :param pmc: PMU events to be enabled for the current profile session

    """
    from collections import OrderedDict
    events = OrderedDict()
    for counter in pmc:
      if isinstance(counter, list):
        for event in counter:
          events.update({event:0})
      else:
        events.update({counter:0})
    return events.keys()

  def resolveTopdownMetrics(self, pmc):
    """
    Resolves pmu events for given topdown metrics.

    The method resolves a list of pmu events, for one or more nodes in the topdown hierarchy

    :param pmc: PMU events to be enabled for the current profile session

    """
    import copy
    from xpedite.pmu.event import TopdownMetrics
    from xpedite.pmu.event import Event, TopdownNode, Metric
    pmc = copy.copy(pmc)
    topdownNodes = [i for i, counter in enumerate(pmc) if isinstance(counter, (TopdownNode, Metric))]
    if topdownNodes:
      topdown = self.topdownCache.get(self.cpuInfo.cpuId)
      self.topdownMetrics = TopdownMetrics()
      for index in topdownNodes:
        node = pmc[index]
        topdownNode = self.topdownMetrics.add(topdown, node)
        pmc[index] = [Event(
          event.name.title().replace('_', '').replace(' ', ''), event.name
        ) for event in topdownNode.events]
    return pmc

class Runtime(AbstractRuntime):
  """Xpedite suite runtime to orchestrate profile session"""

  def __init__(self, app, probes, pmc=None, cpuSet=None, pollInterval=4, samplesFileSize=None, benchmarkProbes=None):
    """
    Creates a new profiler runtime

    Construction of the runtime will execute the following steps
    1. Starts the xpedite app to attach to profiling target
    2. Queries and resolves location of probes in profile info
    3. Load events and topdown database for the target cpu's micro architecture
    4. Resolves pmu events and topdown metrics from events database/topdown hierarchy
    5. Opens xpedite device driver to program pmu events and enable userspace pmc collection
    6. Activates resolved probes and begins sample collection in the target process

    :param app: an instance of xpedite app, to interact with target application
    :type app: xpedite.profiler.app.XpediteApp
    :param probes: List of probes to be enabled for the current profile session
    :param pmc: PMU events to be enabled for the current profile session
    :param cpuSet: List of cpu, where the userspace pmu collection will be enabled
    :type cpuSet: int
    :param pollInterval: Sample collection period in milli seconds
    :type pollInterval: int
    :param samplesFileSize: Max size of data files used to store samples
    :type samplesFileSize: int
    :param benchmarkProbes: optional map to override probes used for benchmarks,
                            defaults to active probes of the current profile session
    """

    from xpedite.dependencies     import Package, DEPENDENCY_LOADER
    DEPENDENCY_LOADER.load(Package.Numpy, Package.FuncTools)
    if len(probes) < 2:
      raise Exception('invalid request - profiling needs at least two named probes to be enabled. Found only {}'.format(
        probes
      ))

    try:
      AbstractRuntime.__init__(self, app, probes)
      self.benchmarkProbes = benchmarkProbes
      self.cpuInfo = app.getCpuInfo()
      eventsDb = self.eventsDbCache.get(self.cpuInfo.cpuId) if pmc else None
      if pmc:
        LOGGER.debug('detected %s', eventsDb.uarchSpec)
        pmc = self.resolveTopdownMetrics(pmc)
        pmc = self.aggregatePmc(pmc)
      if not self.app.dryRun:
        if pmc:
          self.eventSet = self.app.enablePMU(eventsDb, cpuSet, pmc)
        anchoredProbes = self.resolveProbes(probes)
        self.enableProbes(anchoredProbes)
        self.app.beginProfile(pollInterval, samplesFileSize)
      else:
        if pmc:
          self.eventSet = self.resolveEvents(eventsDb, cpuSet, pmc)
        LOGGER.warn('DRY Run selected - xpedite won\'t enable probes')
    except Exception as ex:
      LOGGER.exception('failed to start profiling')
      raise ex

  def report(self, reportName=None, benchmarkPaths=None, classifier=DefaultClassifier(), txnFilter=None, #pylint: disable=too-many-locals
      reportThreshold=3000, resultOrder=ResultOrder.WorstToBest, context=None, ecgwidget=None, stopButton=None):
    """
    Ends active profile session and generates reports.

    This method executes the following steps
    1. Ends samples collection and disconnects tcp connection to target
    2. Gathers sample files for the current profile session and loads elapsed time and pmu counters
    3. Groups related counters to build transactions and timelines
    4. Generates html report and stores results

    :param reportName: Name of the profile report (Default value = None)
    :type reportName: str
    :param benchmarkPaths: List of stored reports from previous runs, for benchmarking (Default value = None)
    :param classifier: Predicate to classify transactions into different categories (Default value = DefaultClassifier()
    :type classifier: xpedite.txn.classifier.ProbeDataClassifier
    :param txnFilter: Lambda to filter transactions prior to report generation
    :type txnFilter: callable accepting a txn instance and returns a bool
    :param reportThreshold: Threshold for number of transactions rendered in html reports (Default value = 3000)
    :type reportThreshold: int
    :param resultOrder: Default sort order of transactions in latency constituent reports
    :type resultOrder: xpedite.pmu.ResultOrder

    """
    from xpedite.profiler.reportgenerator import ReportGenerator
    from xpedite.txn.repo import TxnRepoFactory
    from xpedite.pmu.event       import Event
    try: # pylint: disable=too-many-nested-blocks
      if (not self.app.dryRun) and (self.eventSet):
        self.app.disablePMU()

      repoFactory = TxnRepoFactory()
      pmc = [Event(req.name, req.uarchName) for req in self.eventSet.requests()] if self.eventSet  else []

      timeStart = time.time()

      # dryrun
      if self.app.dataSource:
        repo = repoFactory.buildTxnRepo(None,
          self.app, self.cpuInfo, self.probes, self.topdownCache, self.topdownMetrics,
          pmc, self.benchmarkProbes, benchmarkPaths
        )
        reportName = reportName if reportName else self.app.name
        reportGenerator = ReportGenerator(reportName)
        return reportGenerator.generateReport(
          self.app, repo, classifier, resultOrder, reportThreshold, txnFilter, benchmarkPaths
        )

      # realtime mode
      if context:
        from xpedite.txn.loader           import BoundedTxnLoader
        from xpedite.analytics            import CURRENT_RUN
        from xpedite.txn.filter           import TrivialCounterFilter
        import xpedite.txn.repo
        from xpedite.analytics               import Analytics

        loaderType = BoundedTxnLoader
        loader = loaderType(CURRENT_RUN, self.cpuInfo, self.probes, self.topdownMetrics, events=pmc)
        counterFilter = TrivialCounterFilter()
        analytics = Analytics()

        from xpedite.txn.extractor      import Extractor
        extractor = Extractor(counterFilter)

        firstTimeline = True
        firstTsc = 0
        timeStart = time.time()
        lastEpoch = 0
        timeArr = {}
        minGap = 100

        for loaderSignal in extractor.gatherCountersLive(self.app, loader, stopButton):
          if loaderSignal == 1:
            break
          newTxns = loader.getNewCompleteTxns()
          newRepo = TxnRepoFactory.buildTxnRepo(
            newTxns, self.app, self.cpuInfo, self.probes, self.topdownCache, self.topdownMetrics,
            pmc, None, None
          )
          subCollection = newRepo.getCurrent().getSubCollection()
          if len(subCollection.transactions) == 0:
            continue
          newProfiles = analytics.generateProfiles(reportName, newRepo, classifier)

          for newProfile in newProfiles.profiles:
            timelines = newProfile.current.timelineCollection
            maxLatency = 0
            minLatency = 9999999
            for timeline in timelines:
              tsc = timeline.tsc
              if firstTimeline:
                firstTsc = tsc
                firstTimeline = False
                lastEpoch = int(timeStart*1000)
                lastEpoch -= lastEpoch%minGap
              epoch = int(timeStart*1000) + int(self.cpuInfo.convertCyclesToTime(tsc - firstTsc)/1000)
              if epoch > int(time.time()*1000):
                if epoch - int(time.time()*1000) > 100:
                  LOGGER.warn("collected wrong time stamp")
                epoch = int(time.time()*1000)
              epoch = epoch - epoch%minGap
              if epoch is None:
                LOGGER.warn("missing time stamp")

              if lastEpoch == 0 or abs(epoch - lastEpoch) < minGap:
                for timepoint in timeline.points:
                  latency = timepoint.duration
                  if latency > maxLatency:
                    maxLatency = latency
                  if latency < minLatency and latency != 0:
                    minLatency = latency
                lastEpoch = epoch
                continue

              # if the time gap between the new timeline and the previous timeline is longer than minGap
              # add ticks to the epoch of previous timeline and the min/max latency collected
              if maxLatency != 0 and minLatency != 9999999:
                addTicks(ecgwidget, maxLatency, minLatency, epoch, timeArr)
                # re-initialze min/max latency[]
                maxLatency = 0
                minLatency = 9999999
              for timepoint in timeline.points:
                latency = timepoint.duration
                if latency > maxLatency:
                  maxLatency = latency
                if latency < minLatency and latency != 0:
                  minLatency = latency
                addTicks(ecgwidget, maxLatency, minLatency, epoch, timeArr)
              #update last epoch
              lastEpoch = epoch

            # at the end of this collection, add ticks to last epoch with the min/max collected
            addTicks(ecgwidget, maxLatency, minLatency, epoch, timeArr)

          if not context._profiles: #pylint: disable=protected-access
            context._profiles = newProfiles #pylint: disable=protected-access
          else:
            mergeProfiles(context._profiles, newProfiles) #pylint: disable=protected-access

        # after a stop statement is called and while loop is broke
        loader.endLoad()
        if loader.isCompromised() or loader.getTxnCount() <= 0:
          LOGGER.warn(loader.report())
        elif loader.isNotAccounted():
          LOGGER.debug(loader.report())
        loader.endCollection()
        LOGGER.warn(loader.report())

        allTxns = loader.getData()
        repo = repoFactory.buildTxnRepo(allTxns,
          self.app, self.cpuInfo, self.probes, self.topdownCache, self.topdownMetrics,
          pmc, self.benchmarkProbes, benchmarkPaths
        )
        reportName = reportName if reportName else self.app.name
        reportGenerator = ReportGenerator(reportName)
        return reportGenerator.generateReport(
          self.app, repo, classifier, resultOrder, reportThreshold, txnFilter, benchmarkPaths
        )
      # in batch mode
      else:
        try:
          self.app.endProfile()
        except Exception:
          pass

        repo = repoFactory.buildTxnRepo(
          None, self.app, self.cpuInfo, self.probes, self.topdownCache, self.topdownMetrics,
          pmc, self.benchmarkProbes, benchmarkPaths
        )
        reportName = reportName if reportName else self.app.name
        reportGenerator = ReportGenerator(reportName)
        return reportGenerator.generateReport(
          self.app, repo, classifier, resultOrder, reportThreshold, txnFilter, benchmarkPaths
        )

    except Exception as ex:
      LOGGER.exception('failed to generate report')
      raise ex

def addTicks(ecgwidget, maxLatency, minLatency, epoch, timeArr):
  """
  Add spike to the ECG chart.
  The program maintains a timeArr dictionary to store the maximum and minimum latency
  at each time interval.
  The function executes by the following logic:
    1. check if the epoch for comming data is in the timeArr dictionary
    2. if not, add spike to ECG chart
    3. else, update timeArr and then add spike to ECG chart
  """
  if epoch not in timeArr.keys():
    timeArr[epoch] = (maxLatency, minLatency)
    newDataMax = {'color': 'red',
                  'latency' : maxLatency,
                  'position' : epoch}
    newDataMin = {'color': 'green',
                  'latency' : minLatency,
                  'position' : epoch}
    ecgwidget.evt_ticks = [newDataMax, newDataMin]
  else:
    if maxLatency > timeArr[epoch][0]:
      newDataMax = {'color': 'red',
                    'latency' : maxLatency,
                    'position' : epoch}
      newDataMin = {'color': 'green',
                    'latency' : timeArr[epoch][1],
                    'position' : epoch}
      ecgwidget.evt_ticks = [newDataMax, newDataMin]
      timeArr[epoch] = (maxLatency, timeArr[epoch][1])
    if minLatency < timeArr[epoch][1]:
      newDataMax = {'color': 'red',
                    'latency' : timeArr[epoch][0],
                    'position' : epoch}
      newDataMin = {'color': 'green',
                  'latency' : minLatency,
                  'position' : epoch}
      ecgwidget.evt_ticks = [newDataMax, newDataMin]
      timeArr[epoch] = (timeArr[epoch][0], minLatency)

def mergeProfiles(profiles, newProfiles):
  """
  Merge the new profiles object to the global profiles object
  """
  for newProfile in newProfiles:
    profiles.addProfile(newProfile)
  (profiles.transactionRepo._currentCollection.txnMap).update(newProfiles.transactionRepo._currentCollection.txnMap) #pylint: disable=protected-access
