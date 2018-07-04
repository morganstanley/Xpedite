"""
Report generator

This module provides the following report generation features
  1. Load and categorize transactions
  2. Build latency distribution histograms for each category of transactions
  3. Build html report with (stats, flots, transaction list) for each category, route combination
  4. Generate environment reports

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import time
import numpy
import logging
from xpedite.report.reportbuilder    import ReportBuilder
from xpedite.report.env              import EnvReportBuilder
from xpedite.report.histogram        import (
                                       formatLegend, formatBuckets, buildFlotHistograms,
                                       buildBuckets, buildDistribution, Flot
                                     )
from xpedite.util                    import timeAction, formatHumanReadable
from xpedite.containers              import ProbeMap
from xpedite.report.profile          import Profiles, Profile
from xpedite.analytics               import Analytics, CURRENT_RUN

LOGGER = logging.getLogger(__name__)

class ReportGenerator(object):
  """Generates reports for the current profile session"""

  def __init__(self, reportName):
    """
    Constructs an instance of report generator

    :param reporName: Name of the generated report

    """
    self.reportName = reportName
    self.analytics = Analytics()

  def generateFlots(self, repo, classifier, runId):
    """
    Generates latency distribuion histograms for each category/route combination

    :param repo: Repository of transaction collection
    :type repo: xpedite.transaction.TxnRepo
    :param classifier: Classifier to categorize transactions into various types
    :param runId: Epoch time stamp to uniquely identify a profiling session

    """
    flots = {}
    txnCollections = [repo.getCurrent()] + repo.getBenchmarks().values()
    if not txnCollections[0].isCurrent() or txnCollections[0].name != CURRENT_RUN:
      from xpedite.types import InvariantViloation
      raise InvariantViloation(
        'expecing transactions for current run at index 0 in the repository. '
        'instead found {}'.format(txnCollections[0].name)
      )

    elapsedTimeBundles = self.analytics.buildElapsedTimeBundles(txnCollections, classifier)

    for category, elaspsedTimeBundle in elapsedTimeBundles.iteritems():
      buckets = buildBuckets(elaspsedTimeBundle[0], 35)
      if not buckets:
        LOGGER.debug('category %s has not enough data points to generate flot', category)
        continue

      LOGGER.debug('Buckets:\n%s', buckets)

      yaxis = []
      conflatedCounts = []
      LOGGER.debug('Bucket values:')
      for i, elapsedTimeList in enumerate(elaspsedTimeBundle):
        bucketValues, conflatedCountersCount = timeAction('building counter distribution',
          lambda bkts=buckets, etl=elapsedTimeList: buildDistribution(bkts, etl)
        )
        conflatedCounts.append(conflatedCountersCount)
        LOGGER.debug('%s', bucketValues)
        title = txnCollections[i].name
        legend = formatLegend(
          title, min(elapsedTimeList), max(elapsedTimeList), numpy.mean(elapsedTimeList), numpy.median(elapsedTimeList),
          numpy.percentile(elapsedTimeList, 95), numpy.percentile(elapsedTimeList, 99)
        )
        yaxis.append((legend, bucketValues))

      benchmarkConflatedCounts = sum(conflatedCounts, 1)
      if conflatedCounts[0] + benchmarkConflatedCounts > 0:
        LOGGER.debug(
          'conflation - due to narrow bucket range [%s to %s] - (%d) in current run and (%d) in all '
          'bencmark counter values are conflated',
            buckets[0], buckets[len(buckets)-1],
            conflatedCounts[0], benchmarkConflatedCounts
          )

      buckets = formatBuckets(buckets)
      options, data = buildFlotHistograms(buckets, yaxis, False)
      title = '{} - latency distribution benchmark'.format(category)
      description = 'Latency distribution (current run ID #{} vs chosen benchmarks)'.format(runId)
      flots.update({category: Flot(title, description, data, options)})
    return flots


  @staticmethod
  def getReportProbes(route, userProbes):
    """
    Creates probes with human friendly name for reporting

    :param userProbes: List of probes enabled for a profiling session

    """
    reportProbes = []
    userProbeMap = ProbeMap(userProbes)
    for probe in route.probes:
      if probe in userProbeMap:
        reportProbes.append(userProbeMap[probe])
      else:
        reportProbes.append(probe)
    return reportProbes

  @staticmethod
  def generateEnvironmentReport(app, result, repo, resultOrder, classifier, txnFilter, benchmarkPaths):
    """
    Generates report with environment details

    :param app: an instance of xpedite app, to interact with target application
    :param result: Handle to gather and store profiling results
    :param repo: Repository of loaded transactions
    :param resultOrder: Sort order of transactions in latency constituent reports
    :param classifier: Predicate to classify transactions into different categories
    :param txnFilter: Lambda to filter transactions prior to report generation
    :param benchmarkPaths: List of stored reports from previous runs, for benchmarking

    """
    envReport = EnvReportBuilder().buildEnvironmentReportFile(
      app, repo, resultOrder, classifier, txnFilter, benchmarkPaths
    )
    description = """
    Test environment report (cpu clock frequency, kernel configuration etc.)
    """
    envReportTitle = 'Test Environment Report'
    if envReport:
      result.attachXpediteReport(envReportTitle, envReportTitle, description, envReport)

  @staticmethod
  def addTestResult(reportName, result, timelineStats, benchmarkTlsMap):
    """
    Adds report on perfromance regressions to profile results

    :param reportName: Name of the generated report
    :param result: Handle to gather and store profiling results
    :param timelineStats: Time line statistics for the current run
    :param benchmarkTlsMap: Time line statistics collection for benchmarks

    """
    currentRunMedian = timelineStats.getTotalDurationSeries().getMedian()
    for benchmarkName, benchmarkTls in benchmarkTlsMap.iteritems():
      benchmarkMedian = benchmarkTls.getTotalDurationSeries().getMedian()
      threshold = max(benchmarkMedian * .05, .9)
      result.le(benchmarkMedian + threshold)(
        currentRunMedian, '{} Median latency threshold for current run vs benchmark {}'.format(
          reportName, benchmarkName
        )
      )

  def generateProfiles(self, txnRepo, classifier):
    """
    Generates profiles for the current profile session

    :param txnRepo: Repository of loaded transactions
    :param classifier: Predicate to classify transactions into different categories

    """
    txnTree, benchmarkCompositeTree = self.analytics.buildTxnTree(txnRepo, classifier)
    profiles = Profiles(txnRepo)

    for category, categoryNode in txnTree.getChildren().iteritems():
      i = 1
      for route, txnNode in categoryNode.children.iteritems():
        routeName = ' [route - {}]'.format(i) if len(categoryNode.children) > 1 else ''
        profileName = '{} - {}{}'.format(self.reportName, category, routeName)
        begin = time.time()
        LOGGER.info('generating profile %s (txns - %d) -> ', profileName, len(txnNode.collection))

        benchmarkTxnsMap = benchmarkCompositeTree.getCollectionMap([category, route])
        reportProbes = self.getReportProbes(route, txnRepo.getCurrent().probes)
        timelineStats, benchmarkTimelineStats = self.analytics.computeStats(
          txnRepo, category, route, reportProbes, txnNode.collection, benchmarkTxnsMap
        )
        profiles.addProfile(Profile(profileName, timelineStats, benchmarkTimelineStats))
        elapsed = time.time() - begin
        LOGGER.completed('completed in %0.2f sec.', elapsed)
        i += 1
    return profiles

  def generateLatencyReports(self, profiles, flots, result, resultOrder, reportThreshold):
    """
    Generates latency breakup reports for a list of profiles

    :param profiles: Profile data for the current profile session
    :param flots: Latency distribuion histograms for each category/route combination
    :param result: Handle to gather and store profiling results
    :param resultOrder: Sort order of transactions in latency constituent reports
    :param reportThreshold: Threshold for number of transactions rendered in html reports.

    """
    flotTracker = set()
    for profile in profiles:
      begin = time.time()
      reportTitle = '{} latency statistics [{} transactions]'.format(profile.name, len(profile.current))
      LOGGER.info('generating report %s -> ', reportTitle)

      category = profile.category
      if category not in flotTracker and category in flots:
        flots[category].attach(result)
        flotTracker.add(category)
      self.addTestResult(profile.name, result, profile.current, profile.benchmarks)
      report = ReportBuilder().buildReport(profile.current, profile.benchmarks, profile.reportProbes, profile.name,
        resultOrder, reportThreshold)
      reportSize = formatHumanReadable(len(report))
      reportTitle = '{} - ({})'.format(reportTitle, reportSize)
      description = '\n\t{}\n\t'.format(reportTitle)
      elapsed = time.time() - begin
      LOGGER.completed('completed %s in %0.2f sec.', reportSize, elapsed)
      result.attachXpediteReport(profile.name, reportTitle, description, report)

  def generateReport(self, app, repo, result, classifier, resultOrder, reportThreshold, txnFilter, benchmarkPaths):
    """
    Generates statistics for the current profile session and attaches reports to the given result object

    :param app: An instance of xpedite app, to interact with target application
    :param repo: Repository of transaction collection
    :type repo: xpedite.transaction.TxnRepo
    :param result: Handle to gather and store profiling results
    :param classifier: Predicate to classify transactions into different categories (Default value = DefaultClassifier()
    :param resultOrder: Sort order of transactions in latency constituent reports
    :param reportThreshold: Threshold for number of transactions rendered in html reports.
    :param txnFilter: Lambda to filter transactions prior to report generation
    :param benchmarkPaths: List of stored reports from previous runs, for benchmarking

    """
    try:
      if txnFilter:
        self.analytics.filterTxns(repo, txnFilter)
      flots = self.generateFlots(repo, classifier, app.runId)
      profiles = self.generateProfiles(repo, classifier)
      self.generateLatencyReports(profiles, flots, result, resultOrder, reportThreshold)
      self.generateEnvironmentReport(app, result, repo, resultOrder, classifier, txnFilter, benchmarkPaths)
      LOGGER.info('\nTo recreate the report run - "xpedite report -p profileInfo.py -r %s"\n', app.runId)
      result.commitXpediteReport(app, profiles, self.reportName)
      return profiles
    except Exception as ex:
      LOGGER.exception('failed to generate report')
      raise ex
