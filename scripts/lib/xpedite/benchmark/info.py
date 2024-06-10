"""
Module to create and load benchmark info

This module implements logic to make/load info about a benchmark

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import logging
from datetime               import date
from six.moves              import configparser
from xpedite.types          import CpuInfo
from xpedite.pmu.event      import Event

LOGGER = logging.getLogger(__name__)

BENCHMARK_FILE_NAME = '.benchmark'
BENCHMARK_SECTION = 'info'
BENCHMARK_NAME_KEY = 'name'
BENCHMARK_LEGEND_KEY = 'legend'

BENCHMARK_CPU_INFO_SECTION = 'cpuInfo'
BENCHMARK_CPU_ID_KEY = 'id'
BENCHMARK_CPU_FREQUENCY_KEY = 'frequency'

BENCHMARK_PMC_SECTION = 'pmc'
BENCHMARK_PMC_COUNTER_COUNT = 'counterCount'
BENCHMARK_PMC_COUNTER = 'counter#{}'

def makeBenchmarkInfo(benchmarkName, path, cpuInfo, events=None):
  """
  Creates an info file with benchmark details in human readable format

  :param benchmarkName: Name of the benchmark
  :param path: Path of the benchmark info file
  :param cpuInfo: CPU info for profiles
  :param events: Events associated with a profile

  """
  path = os.path.join(path, BENCHMARK_FILE_NAME)
  config = configparser.RawConfigParser()
  config.add_section(BENCHMARK_SECTION)
  config.set(BENCHMARK_SECTION, BENCHMARK_NAME_KEY, benchmarkName)
  legend = '{} run at {}'.format(benchmarkName, str(date.today()))
  config.set(BENCHMARK_SECTION, BENCHMARK_LEGEND_KEY, legend)

  config.add_section(BENCHMARK_CPU_INFO_SECTION)
  config.set(BENCHMARK_CPU_INFO_SECTION, BENCHMARK_CPU_ID_KEY, cpuInfo.cpuId)
  config.set(BENCHMARK_CPU_INFO_SECTION, BENCHMARK_CPU_FREQUENCY_KEY, cpuInfo.frequency)

  if events:
    config.add_section(BENCHMARK_PMC_SECTION)
    config.set(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER_COUNT, len(events))
    for i, event in enumerate(events):
      value = '{},{},{},{}'.format(event.name, event.uarchName, event.user, event.kernel)
      config.set(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER.format(i), value)
  with open(path, 'w') as configfile:
    config.write(configfile)

def loadBenchmarkInfo(path):
  """
  Loads info about benchmark from file system

  :param path: path of the benchmark info file

  """
  configParser = configparser.RawConfigParser()
  fileName = os.path.join(path, BENCHMARK_FILE_NAME)
  if os.path.exists(fileName):
    configParser.read(fileName)
    benchmarkName = configParser.get(BENCHMARK_SECTION, BENCHMARK_NAME_KEY)
    legend = configParser.get(BENCHMARK_SECTION, BENCHMARK_LEGEND_KEY)

    if not configParser.has_section(BENCHMARK_CPU_INFO_SECTION):
      LOGGER.warning('failed to load benchmark %s - cpu info missing', benchmarkName)
      return None
    cpuId = configParser.get(BENCHMARK_CPU_INFO_SECTION, BENCHMARK_CPU_ID_KEY)
    cpuFrequency = configParser.get(BENCHMARK_CPU_INFO_SECTION, BENCHMARK_CPU_FREQUENCY_KEY)
    cpuInfo = CpuInfo(cpuId, int(cpuFrequency))
    events = None
    if configParser.has_option(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER_COUNT):
      counterCount = configParser.getint(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER_COUNT)
      events = []
      for i in range(counterCount):
        eventStr = configParser.get(BENCHMARK_PMC_SECTION, BENCHMARK_PMC_COUNTER.format(i))
        eventFields = eventStr.split(',')
        events.append(Event(eventFields[0], eventFields[1], bool(eventFields[2]), bool(eventFields[3])))
    return (benchmarkName, cpuInfo, path, legend, events)
  return None
