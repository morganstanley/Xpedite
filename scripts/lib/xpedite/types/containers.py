"""
A multi keyed probe map

This module provides a map, indexed by two different probe attributes
  1. Name of the probe
  2. Location of the probe in source file (file name and line number)

The multi index is used to support both human and machine friendly
probe based lookups.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

class ProbeMap(object):
  """
  A multi-index map to lookup values using name or location of probes

  Builds index for probes system name and source file location
  """

  def __init__(self, probes=None, values=None):
    """
    Constructs a Probe Map

    :param probes: list of probes
    :param values: list of values corresponding to probes
    """
    self.anchoredProbeMap = {}
    self.namedProbeMap = {}
    if probes:
      if values and len(values) < len(probes):
        raise Exception('Argument exception - Failed to build ProbeMap. values({}) not matching probeCount({})'
            .format(len(values), len(probes)))

      for i, probe in enumerate(probes):
        value = values[i] if values else probe
        if probe.isAnchored():
          self.anchoredProbeMap.update({probe:value})
          if probe.sysName:
            self.namedProbeMap.update({probe.sysName:value})
        else:
          self.namedProbeMap.update({probe.sysName:value})

  def __contains__(self, probe):
    if probe.sysName:
      return probe.sysName in self.namedProbeMap
    if probe.isAnchored():
      return probe in self.anchoredProbeMap
    return None

  def get(self, probe, defaultValue):
    """
    Lookup value for a given probe

    :param probe: probe to be used as key for value lookup
    :param defaultValue:  default value, if the probe is not in map

    """
    if probe.sysName:
      return self.namedProbeMap.get(probe.sysName, defaultValue)
    if probe.isAnchored():
      return self.anchoredProbeMap.get(probe, defaultValue)
    return None

  def __getitem__(self, probe):
    if probe.sysName:
      return self.namedProbeMap[probe.sysName]
    if probe.isAnchored():
      return self.anchoredProbeMap[probe]
    return None

  def update(self, probeMap):
    """
    Updates the probe/value mapping from the given map

    :param probeMap: A map of probe to value

    """
    for probe, value in probeMap.items():
      if probe.isAnchored():
        self.anchoredProbeMap.update({probe:value})
        self.namedProbeMap.update({probe.sysName:value})
      else:
        self.namedProbeMap.update({probe.sysName:value})

  def __repr__(self):
    """Returns string representation of a probe map"""

    return 'Probe Map: Achored Probe Map - {} | Named Probe Map - {}'.format(self.anchoredProbeMap, self.namedProbeMap)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__
