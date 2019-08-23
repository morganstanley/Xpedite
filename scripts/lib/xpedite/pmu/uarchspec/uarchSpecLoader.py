#!/usr/bin/env python2.7
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
  'SLM': 'slm_ratios.py',
  'KNL': 'knl_ratios.py',
}

def uarchSpecPath():
  """Returns filesystem location for storing micro architecture specifications"""
  from xpedite.pmu.uarchspec import UARCH_SPEC_PKG_NAME
  return os.path.join(CONFIG.uarchSpecPath, 'uarchSpec', UARCH_SPEC_PKG_NAME)

def manifestFilePath():
  """Returns path to mainifest of known micro architecture specifications"""
  return os.path.join(uarchSpecPath(), CONFIG.manifestFileName)

def makeUarchSpecDir():
  """Creates a directory for storing micro architecture specifications"""
  from xpedite.util import mkdir, touch
  path = uarchSpecPath()
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
  from six.moves.urllib.request import urlopen
  from six.moves.urllib.error import URLError
  try:
    response = urlopen(url)
    data = response.read()
    with open(path, 'w') as fileHandle:
      fileHandle.write(data)
    return True
  except URLError:
    LOGGER.exception('failed to retrieve file "%s" from url - %s', os.path.basename(path), url)
  except IOError:
    LOGGER.exception('failed to open file - %s', path)

def downloadManifest():
  """ Downloads manifest for all known micro architecture specifications from internet"""
  return makeUarchSpecDir() and downloadFile(CONFIG.uarchSpecRepoUrl + CONFIG.manifestFileName, manifestFilePath())

def downloadUarchSpec(uarchSpec):
  """
  Downloads specifications and topdown metrics for a cpu micro architecture

  :param uarchSpec: Name of the cpu micro architecture

  """
  LOGGER.info('\tdownloading uarch spec for %s -> ', uarchSpec.name)
  from xpedite.util import mkdir
  begin = time.time()
  path = os.path.join(uarchSpecPath(), uarchSpec.name)
  mkdir(path)
  url = '{}{}/{}'.format(CONFIG.uarchSpecRepoUrl, uarchSpec.name, os.path.basename(uarchSpec.coreEventsDbFile))
  rc = downloadFile(url, os.path.join(path, uarchSpec.coreEventsDbFile))
  elapsed = time.time() - begin
  LOGGER.completed('completed in %0.2f sec.', elapsed)

  if rc:
    ratiosModuleName = TOPDOWN_RATIOS_MODULES.get(uarchSpec.name)
    if ratiosModuleName:
      url = '{}{}'.format(CONFIG.topdownRatiosRepoUrl, ratiosModuleName)
      LOGGER.info('\tdownloading topdown ratios for %s -> ', uarchSpec.name)
      begin = time.time()
      rc = downloadFile(url, os.path.join(path, '__init__.py'))
      elapsed = time.time() - begin
      uarchSpec.ratiosModule = ''
      LOGGER.completed('completed in %0.2f sec.', elapsed)
  return rc

def downloadUarchSpecDb(uarchSpecDb):
  """
  Downloads specifications for all known cpu micro architectures

  :param uarchSpecDb: Database of specifications for all known cpu micro architectures

  """
  onceFlag = False
  for _, uarchSpec in uarchSpecDb.items():
    if not os.path.exists(uarchSpec.coreEventsDbFile):
      if not onceFlag:
        LOGGER.info('syncing uarch spec database for %d micro architectures', len(uarchSpecDb))
        onceFlag = True
      downloadUarchSpec(uarchSpec)

def loadUarchSpecDb():
  """Loads specifications for all known cpu micro architectures"""
  path = manifestFilePath()
  if not os.path.exists(path) and not downloadManifest():
    return None
  from xpedite.pmu.uarchspec import UarchSpecDb
  uarchSpecDb = UarchSpecDb(path)
  if uarchSpecDb:
    downloadUarchSpecDb(uarchSpecDb)
  sys.path.append(os.path.dirname(uarchSpecPath()))
  return uarchSpecDb

def main():
  """Displays list of pmu events supported by localhost"""
  import argparse
  parser = argparse.ArgumentParser(description='Uarch Spec Loader')
  parser.add_argument('--all', type=str, help='load uarch spec for all available architectures')
  parser.add_argument('--dest', type=str, help='load uarch spec for all available architectures')
  _ = parser.parse_args()
  uarchSpecDb = loadUarchSpecDb()
  if uarchSpecDb:
    LOGGER.info('%s', uarchSpecDb)

if __name__ == '__main__':
  main()
