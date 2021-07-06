#!/usr/bin/env python
"""
UarchSpecLoader collects hardware performance counter specifications
and topdown hierarchy computation modules

This module makes a http request to download micro architectural specificatons
from web servers and caches files for use in configured file system path

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import sys
import time
import logging
from six.moves            import urllib
from xpedite.dependencies import CONFIG

LOGGER = logging.getLogger(__name__)

TOPDOWN_RATIOS_MODULES = {
  'IVB': 'ivb_client_ratios.py',
  'IVT': 'ivb_server_ratios.py',
  'SNB': 'snb_client_ratios.py',
  'JKT': 'jkt_server_ratios.py',
  'HSW': 'hsw_client_ratios.py',
  'HSX': 'hsx_server_ratios.py',
  'BDW': 'bdw_client_ratios.py',
  'BDX': 'bdx_server_ratios.py',
  'SKL': 'skl_client_ratios.py',
  'SKX': 'skx_server_ratios.py',
  'CLX': 'clx_server_ratios.py',
  'SLM': 'slm_ratios.py',
  'KNL': 'knl_ratios.py',
}

def topdownPath():
  """Returns filesystem location for storing topdown modules"""
  from xpedite.pmu.uarchspec import UARCH_SPEC_PKG_NAME
  return os.path.join(CONFIG.uarchSpecPath, 'uarchSpec', UARCH_SPEC_PKG_NAME)

def uarchSpecPath():
  """Returns filesystem location for storing micro architecture specifications"""
  return os.path.normpath(os.path.join(os.path.dirname(__file__), 'data'))

def manifestFilePath():
  """Returns path to mainifest of known micro architecture specifications"""
  return os.path.join(uarchSpecPath(), CONFIG.manifestFileName)

def makeTopdownDir():
  """Creates a directory for storing micro architecture specifications"""
  from xpedite.util import mkdir, touch
  path = topdownPath()
  if os.path.exists(path):
    return path
  try:
    mkdir(path, clean=True)
  except OSError:
    LOGGER.exception('failed to create dir - %s', path)
    return None
  touch(os.path.join(path, '__init__.py'))
  return path

def downloadFile(url, path):
  """
  Downloads micro architecture specifications from internet

  :param url: url of the website hosting the specifications
  :param path: Path of download directory

  """
  import six
  try:
    #pylint: disable=consider-using-with
    connection = urllib.request.urlopen(urllib.request.Request(url), context=CONFIG.sslContext)
    data = connection.read()
    with open(path, 'w') as fileHandle:
      fileHandle.write(six.ensure_str(data))
    return True
  except urllib.error.HTTPError:
    LOGGER.exception('failed to retrieve file "%s" from url - %s', os.path.basename(path), url)
  except IOError:
    LOGGER.exception('failed to open file - %s', path)
  return None

def downloadManifest():
  """ Downloads manifest for all known micro architecture specifications from internet"""
  from xpedite.util import mkdir
  mkdir(os.path.dirname(manifestFilePath()))
  return downloadFile(CONFIG.uarchSpecRepoUrl + CONFIG.manifestFileName, manifestFilePath())

def downloadUarchSpec(uarchSpec):
  """
  Downloads uarc specifications for a cpu micro architecture

  :param uarchSpec: Name of the cpu micro architecture
  """
  from xpedite.util import mkdir
  begin = time.time()
  LOGGER.info('\tdownloading uarch spec for %s -> ', uarchSpec.name)
  path = os.path.dirname(uarchSpec.coreEventsDbFile)
  mkdir(path)
  url = '{}{}/{}'.format(CONFIG.uarchSpecRepoUrl, uarchSpec.name, os.path.basename(uarchSpec.coreEventsDbFile))
  rc = downloadFile(url, os.path.join(path, uarchSpec.coreEventsDbFile))
  elapsed = time.time() - begin
  LOGGER.completed('%s in %0.2f sec.', 'completed' if rc else ' --> failed', elapsed)
  return rc

def downloadtopdownMetrics(uarchSpec):
  """
  Downloads topdown metrics for a cpu micro architecture

  :param uarchSpec: Name of the cpu micro architecture
  """
  from xpedite.util import mkdir
  ratiosModuleName = TOPDOWN_RATIOS_MODULES.get(uarchSpec.name)
  path = os.path.join(topdownPath(), uarchSpec.name)
  if ratiosModuleName and not os.path.exists(path):
    mkdir(path)
    url = '{}{}'.format(CONFIG.topdownRatiosRepoUrl, ratiosModuleName)
    LOGGER.info('\tdownloading topdown ratios for %s -> ', uarchSpec.name)
    begin = time.time()
    rc = downloadFile(url, os.path.join(path, '__init__.py'))
    elapsed = time.time() - begin
    uarchSpec.ratiosModule = ''
    LOGGER.completed('completed in %0.2f sec.', elapsed)
    return rc
  return None

def downloadUarchSpecDb(uarchSpecDb):
  """
  Downloads specifications for all known cpu micro architectures

  :param uarchSpecDb: Database of specifications for all known cpu micro architectures

  """
  onceFlag = False
  makeTopdownDir()
  for _, uarchSpec in uarchSpecDb.items():
    if not os.path.exists(uarchSpec.coreEventsDbFile):
      if not onceFlag:
        LOGGER.info('syncing uarch spec database for %d micro architectures', len(uarchSpecDb))
        onceFlag = True
      downloadUarchSpec(uarchSpec)
    downloadtopdownMetrics(uarchSpec)

def loadUarchSpecDb():
  """Loads specifications for all known cpu micro architectures"""
  path = manifestFilePath()
  if not os.path.exists(path) and not downloadManifest():
    return None
  from xpedite.pmu.uarchspec import UarchSpecDb
  uarchSpecDb = UarchSpecDb(path)
  if uarchSpecDb:
    downloadUarchSpecDb(uarchSpecDb)
  sys.path.append(os.path.dirname(topdownPath()))
  return uarchSpecDb

def main():
  """Displays list of pmu events supported by localhost"""
  import argparse
  import logger
  global LOGGER # pylint: disable=global-statement
  LOGGER = logging.getLogger('xpedite')
  logger.init()
  parser = argparse.ArgumentParser(description='Uarch Spec Loader')
  parser.add_argument('--all', type=str, help='load uarch spec for all available architectures')
  parser.add_argument('--dest', type=str, help='load uarch spec for all available architectures')
  _ = parser.parse_args()
  uarchSpecDb = loadUarchSpecDb()
  if uarchSpecDb:
    LOGGER.info('%s', uarchSpecDb)

if __name__ == '__main__':
  main()
