"""
Util class to deal with nuances in PyCpuInfo API

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import json

from xpedite.dependencies import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.PyCpuInfo)

class CpuInfo(object):
  """
  Builds a map of /proc/cpuinfo to derive id and advertised frequecy for a cpu
  """

  def __init__(self, info=None):
    """Loads cpu info from localhost"""
    from cpuinfo import cpuinfo
    self.info = info if info else cpuinfo.get_cpu_info()
    self.cpuId = self._loadId()
    self.advertisedHz = self._loadAdvertisedHz()

  def _loadId(self):
    """Returns the cpu identifier from vendor, family, model and stepping"""
    vendorId = self.info.get('vendor_id')
    vendorId = vendorId if vendorId else self.info.get('vendor_id_raw')
    stepping = self.info.get('stepping') if 'stepping' in self.info else None
    if vendorId:
      vendorIdString =  '{}-{}-{:02X}'.format(vendorId, self.info['family'], self.info['model'])
      if stepping:
        vendorIdString += '-{}'.format(stepping)
      return vendorIdString
    raise Exception('failed to load cpuInfo - missing cpu vendor id\n{}'.format(self.info))

  def _loadAdvertisedHz(self):
    """Returns cpu advertised frequency"""
    advertisedHz = self.info.get('hz_advertised_raw')
    advertisedHz = advertisedHz if advertisedHz else self.info.get('hz_advertised')
    if not advertisedHz:
      raise Exception('failed to load cpuInfo - missing hz_advertised\n{}'.format(self.info))
    return int(advertisedHz[0])

  def items(self):
    """Returns a dict of cpu info attributes"""
    return self.info.items()

  def __repr__(self):
    return 'Cpu {} | advertised frequency - {}\n{}'.format(self.cpuId, self.advertisedHz, self.info)

class CpuInfoJSONEncoder(json.JSONEncoder):
  """
  Encodes CpuInfo to json format
  """

  def default(self, o): #pylint: disable=E0202
    """Encodes the underlying info, ignoring other fields"""
    if isinstance(o, CpuInfo):
      return o.info
    return super(CpuInfoJSONEncoder, self).default(o)

def decodeCpuInfo(dct):
  """Decodes CpuInfo from a dict"""
  if 'family' in dct and 'model' in dct and 'stepping' in dct:
    return CpuInfo(dct)
  return dct
