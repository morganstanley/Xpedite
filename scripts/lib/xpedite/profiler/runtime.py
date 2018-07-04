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
    self.profiles = None
    self.probeResolver = ProbeResolver()
    self.cpuInfo = None
    self.eventsDbCache = EventsDbCache()
    self.topdownCache = TopdownCache(self.eventsDbCache)
    self.topdownMetrics = None
    self.eventState = None

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
      if self.eventState:
        errMsg = ProbeAdmin.enablePMU(self.app, self.eventState)
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
    return PMUCtrl(eventsDb).resolveEvents(cpuSet, events)

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

  def initTopdown(self, pmc):
    """
    Resolves pmu events for given topdown metrics.

    The method resolves a list of pmu events, for one or more nodes in the topdown hierarchy

    :param pmc: PMU events to be enabled for the current profile session

    """
    from xpedite.pmu.event import TopdownMetrics
    from xpedite.pmu.event import Event, TopdownNode, Metric
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

class Runtime(AbstractRuntime):
  """Xpedite suite runtime to orchestrate profile session"""

  def __init__(self, app, probes, pmc=None, cpuSet=None, pollInterval=4, benchmarkProbes=None):
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
        self.initTopdown(pmc)
        pmc = self.aggregatePmc(pmc)
      if not self.app.dryRun:
        if pmc:
          self.eventState = self.app.enablePMU(eventsDb, cpuSet, pmc)
        anchoredProbes = self.resolveProbes(probes)
        self.enableProbes(anchoredProbes)
        self.app.beginProfile(pollInterval)
      else:
        if pmc:
          self.eventState = self.resolveEvents(eventsDb, cpuSet, pmc)
        LOGGER.warn('DRY Run selected - xpedite won\'t enable probes')
    except Exception as ex:
      LOGGER.exception('failed to start profiling')
      raise ex

  def report(self, result, reportName=None, benchmarkPaths=None, classifier=DefaultClassifier(), txnFilter=None,
      reportThreshold=3000, resultOrder=ResultOrder.WorstToBest, buildPrefix=None):
    """
    Ends active profile session and generates reports.

    This method executes the following steps
    1. Ends samples collection and disconnects tcp connection to target
    2. Gathers sample files for the current profile session and loads elapsed time and pmu counters
    3. Groups related counters to build transactions and timelines
    4. Generates html report and stores results

    :param result: Handle to collect and store profile results
    :type result: xpedite.jupyter.Result
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
    :param buildPrefix: Build prefix to be trimmed source file paths of probes
    :type buildPrefix: str

    """
    from xpedite.profiler.reportgenerator import ReportGenerator
    from xpedite.txn.repo import TxnRepoFactory
    from xpedite.pmu.event       import Event
    try:
      if not self.app.dryRun:
        try:
          self.app.endProfile()
        except Exception:
          pass
        if self.eventState:
          self.app.disablePMU()

      repoFactory = TxnRepoFactory(buildPrefix)
      pmc = [Event(req.name, req.uarchName) for req in self.eventState.requests()] if self.eventState  else []
      repo = repoFactory.buildTxnRepo(
        self.app, self.cpuInfo, self.probes, self.topdownCache, self.topdownMetrics,
        pmc, self.benchmarkProbes, benchmarkPaths
      )
      reportName = reportName if reportName else self.app.name
      reportGenerator = ReportGenerator(reportName)
      self.profiles = reportGenerator.generateReport(
        self.app, repo, result, classifier, resultOrder, reportThreshold, txnFilter, benchmarkPaths
      )
    except Exception as ex:
      LOGGER.exception('failed to generate report')
      raise ex

  def makeBenchmark(self, path):
    """
    Persists samples for current run in the given path for future benchmarking

    :param path: Path to persist profiles for the current session

    """
    from xpedite import benchmark
    return benchmark.makeBenchmark(self.profiles, path)
