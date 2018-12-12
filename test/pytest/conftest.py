"""
Configuration for pytest tests, enables passing of a command line argument to the
runTest.sh script to specify the hostname to run a target application on

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
import pytest

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

@pytest.fixture
def hostname(request):
  """
  The hostname of a remote host to run a target application on
  """
  return request.config.option.hostname

@pytest.fixture
def transactions(request):
  """
  The number of transactions a target application should create
  """
  return request.config.option.transactions

@pytest.fixture
def multithreaded(request):
  """
  The number of threads to use in the target application
  """
  return request.config.option.multithreaded

@pytest.fixture
def workspace(request):
  """
  The number of threads to use in the target application
  """
  return request.config.option.workspace

@pytest.fixture
def rundir(request):
  """
  Directory to extract files to
  """
  return request.config.option.rundir

@pytest.fixture
def apps(request):
  """
  Directory to extract files to
  """
  apps = request.config.option.apps
  return apps.split(',')
