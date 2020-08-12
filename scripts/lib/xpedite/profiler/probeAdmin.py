"""
ProbeAdmin to query and update probe state

This module provide functionality to
  1. Query the list of instrumented probes and their current status
  2. Activate/Deactivate a probe
  3. Configure collection of performance counter

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
from xpedite.util.probeFactory import ProbeFactory

class ProbeAdmin(object):
  """Utility class to administer target process - query/enable/disable probes"""

  @staticmethod
  def targetStateStr(targetState):
    """Returns the string for probe target state boolean"""
    return 'enable' if targetState else 'disable'

  @staticmethod
  def getProbes(app):
    """
    Fetches the list of instrumeted probes and their respective status from target process

    :param app: Handle to an instance of the xpedite app
    :type app: xpedite.profiler.app.XpediteApp

    """
    cmd = 'ListProbes'
    result = app.admin(cmd, timeout=10)
    if result:
      result = result.strip()
      return list(ProbeFactory(app.workspace).buildFromRecords(result.split('\n')).values())
    raise Exception('failed to query probes - have you instrumentd any xpedite probes in your binary ?')

  @staticmethod
  def _updateProbe(app, anchoredProbe, targetState):
    """
    Updates state of the given probe in the target process

    :param app: Handle to an instance of the xpedite app
    :type app: xpedite.profiler.app.XpediteApp
    :param anchoredProbe: The probe to activate/deactive
    :type anchoredProbe: xpedite.types.probe.AnchoredProbe
    :param targetState: Activation/deactivaatione flag for the given probe
    :type targetState: bool

    """
    probeFilePath = os.path.basename(anchoredProbe.filePath)
    cmd = 'ActivateProbe' if targetState else 'DeactivateProbe'
    cmd += ' --file {} --line {}'.format(probeFilePath, anchoredProbe.lineNo)
    return app.admin(cmd, timeout=10)

  @staticmethod
  def updateProbes(app, anchoredProbes, targetState):
    """
    Updates state of the given list of probes in the target process

    :param app: Handle to an instance of the xpedite app
    :type app: xpedite.profiler.app.XpediteApp
    :param anchoredProbes: A list of probes to activate/deactive
    :param targetState: Activation/deactivaatione flag for the given list of probes
    :type targetState: bool

    """

    for probe in anchoredProbes:
      ProbeAdmin._updateProbe(app, probe, targetState)

    errCount = 0
    errMsg = ''
    probes = ProbeAdmin.getProbes(app)

    if probes:
      probeMap = {}
      for probe in probes:
        probeMap.update({probe:probe.isActive})

      for probe in anchoredProbes:
        if probe in probeMap:
          if probeMap[probe] != targetState:
            if errCount > 0:
              errMsg += '\n'
            errMsg += 'failed to {} probe {}'.format(ProbeAdmin.targetStateStr(targetState), probe)
            errCount += 1
        else:
          if errCount > 0:
            errMsg += '\n'
          errMsg += 'failed to enable probe {0}. Please make sure, {0} is a valid probe'.format(probe)
          errCount += 1

    return (errCount, errMsg)

  @staticmethod
  def enablePMU(app, eventSet):
    """
    Enables PMU and updates the usage count of general and fixed purpose pmu registers

    :param app: an instance of xpedite app, to interact with target application
    :param eventSet: Handle to state of PMU events for the current profile session

    """
    gpPmcOption = '--gpCtrCount {}'.format(eventSet.genericRequestCount()) if eventSet.genericRequestCount() else ''
    fixedPmcOption = ''
    if eventSet.fixedRequestCount():
      fixedPmcOption = '--fixedCtrList {}'.format(','.join([str(event.ctrIndex) for event in eventSet.fixedRequests]))
    cmd = 'ActivatePmu {} {}'.format(gpPmcOption, fixedPmcOption)
    return app.admin(cmd, timeout=10)

  @staticmethod
  def loadProbes(app):
    """
    Builds a map of probe states from target application

    :param app: an instance of xpedite app, to interact with target application

    """
    from xpedite.types.containers import ProbeMap
    probes = ProbeAdmin.getProbes(app)
    return list(ProbeMap(probes, probes).namedProbeMap.values())
