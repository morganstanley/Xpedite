"""
Configuration for pytest tests, enables passing of a command line argument to the
runTest.sh script to specify the hostname to run a target application on

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import pytest

def pytest_addoption(parser):
  parser.addoption('--hostname', action='store', default='127.0.0.1', help='enter remote host')

@pytest.fixture
def hostname(request):
  return request.config.option.hostname
