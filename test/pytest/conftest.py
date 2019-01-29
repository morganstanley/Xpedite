"""
Configuration for pytest tests, enables passing of a command line argument to the
runTest.sh script to specify the hostname to run a target application on

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
import pytest
from test_xpedite.test_pmu               import CPU_IDS
from test_xpedite.test_profiler.scenario import ScenarioType

APPS_ARG = 'apps'
SCENARIOS_ARG = 'scenarioTypes'
PARAMETER_SCENARIO = 'scenarioName'
PARAMETER_CPU_ID = 'cpuId'

def pytest_addoption(parser):
  """
  Parse options from runTest.sh script to be used for pytest tests
  """
  workspaceDefault = os.path.join(os.path.abspath(__file__).split('/test/')[0], '')
  parser.addoption('--hostname', action='store', default='127.0.0.1', help='enter remote host')
  parser.addoption('--transactions', action='store', default=1024, help='enter number of transactions to run')
  parser.addoption('--multithreaded', action='store', default=1, help='enter number of threads')
  parser.addoption('--workspace', action='store', default=workspaceDefault, help='workspace path to trim from file paths')
  parser.addoption('--rundir', action='store', default='', help='directory to extract files to')
  parser.addoption('--apps', action='store', default='[slowFixDecoderApp]', help='apps to test')
  parser.addoption('--scenarioTypes', action='store', default='[Regular, Benchmark, PMC]', help='scenarios to run')
  parser.addoption('--recordPMC', action='store', default=False, help='record PMC during testing')

def pytest_generate_tests(metafunc):
  """
  Parametrize pytests by scenario name
  """
  if PARAMETER_SCENARIO in metafunc.fixturenames:
    scenarioNames = []
    for app in metafunc.config.getoption(APPS_ARG).split(','):
      for scenario in metafunc.config.getoption(SCENARIOS_ARG).split(','):
        scenarioNames.append('{}{}'.format(app, scenario))
    metafunc.parametrize(PARAMETER_SCENARIO, scenarioNames)
  if PARAMETER_CPU_ID in metafunc.fixturenames:
    metafunc.parametrize(PARAMETER_CPU_ID, CPU_IDS)

@pytest.fixture(scope='module')
def hostname(request):
  """
  The hostname of a remote host to run a target application on
  """
  return request.config.option.hostname

@pytest.fixture(scope='module')
def transactions(request):
  """
  The number of transactions a target application should create
  """
  return request.config.option.transactions

@pytest.fixture(scope='module')
def multithreaded(request):
  """
  The number of threads to use in the target application
  """
  return request.config.option.multithreaded

@pytest.fixture(scope='module')
def workspace(request):
  """
  Workspace path to trim from file paths
  """
  return request.config.option.workspace

@pytest.fixture(scope='module')
def rundir(request):
  """
  Directory to extract files to
  """
  return request.config.option.rundir

@pytest.fixture(scope='module')
def apps(request):
  """
  Apps to test
  """
  return request.config.option.apps.split(',')

@pytest.fixture(scope='module')
def scenarioTypes(request):
  """
  Scenarios to run
  """
  scenarioTypes = []
  for scenarioType in request.config.option.scenarioTypes.split(','):
    scenarioTypes.append(ScenarioType[scenarioType])
  return scenarioTypes

@pytest.fixture(scope='module')
def recordPMC(request):
  """
  Record PMC during testing
  """
  return request.config.option.recordPMC
