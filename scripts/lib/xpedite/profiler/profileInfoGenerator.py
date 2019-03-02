"""
Module to auto generate profile info
This module provides logic, to generate a python module (profileInfo.py) on demand.

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""
import os
import re
import logging
from tempfile import mkstemp

LOGGER = logging.getLogger(__name__)

class ProfileInfoGenerator(object):
  """Generator to build profile info module for a target application"""

  probeNameFormatter = re.compile('[A-Z0-9]?[a-z]+|[A-Z0-9]+(?=[A-Z]|)')
  replaceAppName = re.compile('(appName = )(.*)(\')')
  replaceAppHost = re.compile('(appHost = )(.*)(\')')
  replaceAppInfo = re.compile('(appInfo = )(.*)(\')')
  replaceProbes = re.compile('(probes =)(.*)(\\n])', re.DOTALL)

  def __init__(self, appName, appHost, appInfoPath, probes, profilerPath):
    self.appName = appName
    self.appHost = appHost
    self.appInfoPath = appInfoPath
    self.probes = probes
    self.probes.sort(key=lambda p: p.getCanonicalName())
    self.pFile = None
    self.profilerPath = profilerPath
    self.filePath = os.getcwd() + '/profileInfo.py'

  def generate(self):
    """Generates the profile info module for the target process"""
    hasTempFile = False
    if os.path.isfile(self.filePath):
      _, self.filePath = mkstemp()
      hasTempFile = True

    with open(self.filePath, 'w') as self.pFile:
      appNameStr = 'appName = \'{}\''.format(self.appName)
      appHostStr = 'appHost = \'{}\''.format(self.appHost)
      appInfoStr = 'appInfo = \'{}\''.format(self.appInfoPath)

      data = self.loadTemplateFile()
      data = re.sub(self.replaceAppName, appNameStr, data)
      data = re.sub(self.replaceAppHost, appHostStr, data)
      data = re.sub(self.replaceAppInfo, appInfoStr, data)

      probeDataStr = self.generateProbes()
      data = re.sub(self.replaceProbes, probeDataStr, data)
      self.pFile.write(data)

    if hasTempFile:
      LOGGER.completed('Created temporary file %s with %d probes', self.filePath, len(self.probes))

    generateMessage = """
The profile info is a python module, that can be used to record a profile.
It contains the following:
1. List of probes in {}
2. List of performance counters to enable
3. Facilities to classify, filter, and sort transactions in reports

For more details check http://xpedite. Feel free to edit the file to
enable/disable probes or fine tune profile parameters.

To record a profile run "{} record -p {}"
""".format(self.appName, self.profilerPath, self.filePath)
    LOGGER.info(generateMessage)
    return self.filePath

  def generateNames(self, probe):
    """Generates human friendly name for a probe from it's sys name"""
    sysName = probe.sysName
    split = re.findall(self.probeNameFormatter, sysName)
    displayName = ' '.join(split)
    displayName = displayName.replace('_', ' ')
    return sysName, displayName

  def generateProbes(self):
    """Generates code for a list of instrumented probes"""
    probeDataStr = 'probes = [\n'
    for probe in self.probes:
      if probe.canBeginTxn:
        probeType = 'TxnBeginProbe'
      elif probe.canSuspendTxn:
        probeType = 'TxnSuspendProbe'
      elif probe.canResumeTxn:
        probeType = 'TxnResumeProbe'
      elif probe.canEndTxn:
        probeType = 'TxnEndProbe'
      else:
        probeType = 'Probe'
      sysName, displayName = self.generateNames(probe)
      probeDataStr += ('  {}(\'{}\', sysName = \'{}\'),\n'.format(probeType, displayName, sysName))
    probeDataStr += (']')
    return probeDataStr

  @staticmethod
  def loadTemplateFile():
    """Loads the template file for generating profile info"""
    templatePath = os.path.join(os.path.dirname(__file__), '../../examples/profileInfo.py')
    try:
      with open(templatePath, 'r') as tempFile:
        fileData = tempFile.read()
        return fileData
    except IOError:
      ioErr = 'Could not read template file from ' + templatePath
      raise Exception(ioErr)
