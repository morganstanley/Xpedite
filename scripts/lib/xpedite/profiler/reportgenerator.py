"""
Report generator

This module provides the following report generation features
  1. Load and categorize transactions
  2. Build latency distribution histograms for each category of transactions
  3. Build html report with (stats, histograms, transaction list) for each category, route combination
  4. Generate environment reports

Author: Manikandan Dhamodharan, Morgan Stanley
"""
import numpy
import logging
import xpedite.report
from xpedite.report.histogram        import (
                                       formatLegend, formatBuckets, buildHistograms,
                                       buildBuckets, buildDistribution, Histogram
                                     )
from xpedite.util                    import timeAction
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

  def generateHistograms(self, repo, classifier, runId):
    """
    Generates latency distribuion histograms for each category/route combination

    :param repo: Repository of transaction collection
    :type repo: xpedite.txn.repo.TxnRepo
    :param classifier: Classifier to categorize transactions into various types
    :param runId: Epoch time stamp to uniquely identify a profiling session

    """
    histograms = {}
    txnCollections = [repo.getCurrent()] + list(repo.getBenchmarks().values())
    if not txnCollections[0].isCurrent() or txnCollections[0].name != CURRENT_RUN:
      from xpedite.types import InvariantViloation
      raise InvariantViloation(
        'expecing transactions for current run at index 0 in the repository. '
        'instead found {}'.format(txnCollections[0].name)
      )

    elapsedTimeBundles = self.analytics.buildElapsedTimeBundles(txnCollections, classifier)

    for category, elaspsedTimeBundle in elapsedTimeBundles.items():
      buckets = buildBuckets(elaspsedTimeBundle[0], 35)
      if not buckets:
        LOGGER.debug('category %s has not enough data points to generate histogram', category)
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
      options, data = buildHistograms(buckets, yaxis, False)
      title = '{} - latency distribution benchmark'.format(category)
      description = 'Latency distribution (current run ID #{} vs chosen benchmarks)'.format(runId)
      histograms.update({category: Histogram(title, description, data, options)})
    return histograms

  def generateReport(self, app, repo, classifier, resultOrder, reportThreshold, txnFilter, benchmarkPaths,
          routeConflation):
    """
    Generates report for the current profile session

    :param app: An instance of xpedite app, to interact with target application
    :param repo: Repository of transaction collection
    :type repo: xpedite.txn.repo.TxnRepo
    :param classifier: Predicate to classify transactions into different categories
    :param resultOrder: Sort order of transactions in latency constituent reports
    :param reportThreshold: Threshold for number of transactions rendered in html reports.
    :param txnFilter: Lambda to filter transactions prior to report generation
    :param benchmarkPaths: List of stored reports from previous runs, for benchmarking
    :param routeConflation: Parameter to control, whether routes can be conflated or not

    """
    try:
      if txnFilter:
        self.analytics.filterTxns(repo, txnFilter)
      histograms = self.generateHistograms(repo, classifier, app.runId)
      profiles = self.analytics.generateProfiles(self.reportName, repo, classifier, routeConflation)
      report = xpedite.report.generate(
        app, profiles, histograms, resultOrder, classifier, txnFilter, benchmarkPaths, reportThreshold
      )
      LOGGER.info('\nTo recreate the report run - "xpedite report -p profileInfo.py -r %s"\n', app.runId)
      return report
    except Exception as ex:
      LOGGER.exception('failed to generate report')
      raise ex
