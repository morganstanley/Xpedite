"""
Module to check properties of an ip address

Author: Manikandan Dhamodharan, Morgan Stanley
"""

def ip4Addresses():
  """Returns ip addresses for all interfaces in a machine"""
  from netifaces import interfaces, ifaddresses, AF_INET # pylint: disable=no-name-in-module
  ipSet = set()
  for interface in interfaces():
    addrInfo = ifaddresses(interface)
    if AF_INET in addrInfo:
      for link in addrInfo[AF_INET]:
        ipSet.add(link['addr'])
  return ipSet

def isIpLocal(ip):
  """Checks if a given address is a loopback or remote ip"""
  if ip.lower() == 'localhost':
    return True
  ipSet = ip4Addresses()
  return ip in ipSet
