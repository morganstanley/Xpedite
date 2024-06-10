"""
Module to support specification of xpedite profiling parameters.

This module provides functionality to build profileInfo object from a
user supplied python module.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import sys
import importlib.util
import logging

LOGGER = logging.getLogger(__name__)

class ProfileInfo(object):
  """Profile info stores settings and parameters to control profiling and report generation."""

  def __init__(self, appName, appHost, appInfo, probes, homeDir, pmc,
    cpuSet, benchmarkPaths, classifier, resultOrder, txnFilter, routeConflation):
    """
    Constructs an instance of ProfileInfo

    :param appName: Name of the target application
    :param appHost: Host, where the applciation is running
    :param appInfo: Path to the appinfo file of the target application
    :param probes: List of probes to be enabled, for the current profile session
    :param homeDir: Home directory to store xpedite reports
    :param pmc: List of programmable pmu events for the current profile session
    :param cpuSet: List of cpu, where userspace pmu collection will be enabled
    :param benchmarkPaths: List of stored reports from previous runs for benchmarking
    :param classifier: Predicate to classify transactions into different categories
    :param resultOrder: Default sort order for transactions in latency constituent reports
    :type resultOrder: xpedite.pmu.ResultOrder
    :param txnFilter: Lambda to filter transactions prior to report generation
    :param routeConflation: Parameter to control, whether routes can be conflated or not
    :type routeConflation: xpedite.types.RouteConflation

    """
    self.appName = appName.replace(' ', '_')
    self.appHost = appHost
    self.appInfo = appInfo
    self.homeDir = homeDir
    self.probes = probes
    self.pmc = pmc
    self.cpuSet = cpuSet
    self.benchmarkPaths = benchmarkPaths
    self.classifier = classifier
    self.resultOrder = resultOrder
    self.txnFilter = txnFilter
    self.routeConflation = routeConflation

  def __repr__(self):
    strRepr = 'app name = {}, appHost = {}, appInfo = {}\n'.format(self.appName, self.appHost, self.appInfo)
    strRepr += 'probes = {}\n'.format(self.probes)
    strRepr += 'pmc = {}, cpuSet = {}\n'.format(self.pmc, self.cpuSet)
    return strRepr

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

def loadProfileInfo(profilePath):
  """
  Loads Profile info object from a python module

  :param profilePath: Path to the profile info module

  """
  if not os.path.isfile(profilePath):
    LOGGER.error('cannot load profile from path - "%s"\nPlease make sure the file exists.', profilePath)
    sys.exit(1)
  try:
    path = os.path.abspath(profilePath)
    fileName = os.path.split(profilePath)[1]
    moduleName = str.split(fileName, '.')[0]
    spec = importlib.util.spec_from_file_location(moduleName, path)
    profileInfo  = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(profileInfo)
    benchmarkPaths = getattr(profileInfo, 'benchmarkPaths', None)
    pmc = getattr(profileInfo, 'pmc', None)
    cpuSet = getattr(profileInfo, 'cpuSet', None)
    classifier = getattr(profileInfo, 'classifier', None)
    resultOrder = getattr(profileInfo, 'resultOrder', None)
    homeDir = getattr(profileInfo, 'homeDir', None)
    txnFilter = getattr(profileInfo, 'txnFilter', None)
    routeConflation = getattr(profileInfo, 'routeConflation', None)
    return ProfileInfo(profileInfo.appName, profileInfo.appHost, profileInfo.appInfo,
      profileInfo.probes, homeDir, pmc, cpuSet, benchmarkPaths, classifier, resultOrder, txnFilter, routeConflation)
  except Exception:
    LOGGER.exception('failed to load profile file "%s"', profilePath)
    sys.exit(2)
