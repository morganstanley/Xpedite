"""
Probe Resolver

This module is used to convert probes in human friendly format to machine friendly format.
It provides a lookup to translate, a list of probe names, to achored probe objects.

Author: Manikandan Dhamodharan, Morgan Stanley

"""
from xpedite.profiler.probeAdmin import ProbeAdmin

class ProbeResolver(object):
  """Utility class to resolve location of probes in application source files"""

  def __init__(self):
    self.anchoredProbeMap = None

  def resolveAnchoredProbe(self, app, probe):
    """
    Resolves source location of the given probe

    :param app: Handle to the instance of the xpedite app
    :type app: xpedite.profiler.app.XpediteApp
    :param probe: handle to a probe for resolution of file name and line number
    :type probe: xpedite.types.probe.AnchoredProbe

    """
    if self.anchoredProbeMap is None:
      self.anchoredProbeMap = {}
      probes = ProbeAdmin.getProbes(app)
      for liveProbe in probes:
        if liveProbe.sysName:
          if liveProbe.sysName in self.anchoredProbeMap:
            self.anchoredProbeMap[liveProbe.sysName].append(liveProbe)
          else:
            self.anchoredProbeMap.update({liveProbe.sysName:[liveProbe]})

    if self.anchoredProbeMap and probe.sysName in self.anchoredProbeMap:
      return self.anchoredProbeMap[probe.sysName]
    return None
