"""
Probe definitions

This module defines the following probe types
  1. Probe - Probes that store transaction id
  2. TxnBeginProbe - Probes that mark the beginning of a transaction
  3. TxnEndProbe - Probes that mark the end of a transaction
  4. AnonymousProbe - Probes that doesn't carry transaction id data
  5. AnchoredProbe -  Probes identified by their location in source code

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
from six.moves import intern

class AbstractProbe(object):
  """Base class for probes"""

  def __init__(self, name):
    self.name = name
    self.isAnonymous = False
    self.canBeginTxn = False
    self.canSuspendTxn = False
    self.canResumeTxn = False
    self.canEndTxn = False
    self.isActive = None

class Probe(AbstractProbe):
  """
  NamedProbe

  Probe objects must be initialized with a valid sysName
  """

  def __init__(self, name, sysName):
    AbstractProbe.__init__(self, name)
    if not sysName:
      raise Exception('Argument exception - failed to create probe {}. must have sysName'.format(name))
    self.sysName = sysName

  @staticmethod
  def isAnchored():
    """Defaults to false"""
    return False

  def __hash__(self):
    return hash(self.sysName)

  def __eq__(self, other):
    return self.sysName == other.sysName

  def __ne__(self, other):
    return not self.__eq__(other)

  def getCanonicalName(self):
    """Returns sysName as canonical name"""
    return '{}'.format(self.sysName)

  def __repr__(self):
    """Returns string representation of a probe"""
    return 'Probe[{}]'.format(self.name)

class AnonymousProbe(Probe):
  """Named probes missing transaction identifier"""

  def __init__(self, name, sysName):
    Probe.__init__(self, name, sysName)
    self.isAnonymous = True

  def __repr__(self):
    return 'AnonymousProbe@' + self.getCanonicalName()

class AnchoredProbe(AbstractProbe):
  """Probes identified by their location in source code - file name and line number"""

  def __init__(self, name, filePath, lineNo, attributes, isActive, sysName=None):
    """
    Constructs an anchored probe

    :param name: User friendly name for use in reports
    :param filePath: Path of the source file containing the probe
    :param lineNo: Line number of the statement containing the probe
    :param attributes: Attributes associated with this probe
    :param sysName: Name of the probe, as defined in the instrumented source file
    """
    AbstractProbe.__init__(self, name)
    if not (filePath and lineNo):
      errMsg = """Argument exception - failed to build anchored probe {}.
        must have valid value for <filePath> and <lineNo>""".format(name)
      raise Exception(errMsg)

    self.sysName = sysName
    self.filePath = intern(filePath)
    self.path = intern(os.path.dirname(filePath))
    self.fileName = intern(os.path.basename(filePath))
    self.lineNo = int(lineNo)
    self.attributes = attributes
    self.isActive = isActive
    self.initAttributes()

  def initAttributes(self):
    """Initializes attributes associated with this probe"""
    for attr in self.attributes.split(','):
      setattr(self, attr, True)

  @staticmethod
  def isAnchored():
    """Defaults to True"""
    return True

  def __hash__(self):
    return hash(self.fileName) ^ hash(self.lineNo)

  def __eq__(self, other):
    if not other.isAnchored():
      from xpedite.types import InvariantViloation
      raise InvariantViloation('only anchored probes can be compared')

    if self.lineNo != other.lineNo or self.fileName != other.fileName:
      return False

    if self.path and other.path:
      longPath, shortPath = (self.path, other.path) if len(self.path) >= len(other.path) else (other.path, self.path)
      return longPath.find(shortPath, len(longPath) - len(shortPath)) != -1
    return True

  def __ne__(self, other):
    return not self.__eq__(other)

  def getCanonicalName(self):
    """Returns sysname, if present else the location of probe in source file"""
    if self.sysName:
      return self.sysName
    return '{}:{}'.format(self.fileName, self.lineNo)

  def __repr__(self):
    """Returns string representation of a probe"""
    return 'AnchoredProbe@' + self.getCanonicalName()

class TxnBeginProbe(Probe):
  """MarkerProbe to begin transaction"""
  def __init__(self, name, sysName):
    Probe.__init__(self, name, sysName)
    self.canBeginTxn = True

  def __repr__(self):
    return 'TxnBeginProbe@' + self.getCanonicalName()

class TxnSuspendProbe(Probe):
  """MarkerProbe to begin transaction"""
  def __init__(self, name, sysName):
    Probe.__init__(self, name, sysName)
    self.canSuspendTxn = True

  def __repr__(self):
    return 'TxnSuspendProbe@' + self.getCanonicalName()

class TxnResumeProbe(Probe):
  """MarkerProbe to end transaction"""
  def __init__(self, name, sysName):
    Probe.__init__(self, name, sysName)
    self.canResumeTxn = True

  def __repr__(self):
    return 'TxnResumeProbe@' + self.getCanonicalName()

class TxnEndProbe(Probe):
  """MarkerProbe to end transaction"""
  def __init__(self, name, sysName):
    Probe.__init__(self, name, sysName)
    self.canEndTxn = True

  def __repr__(self):
    return 'TxnEndProbe@' + self.getCanonicalName()

def compareProbes(lhs, rhs):
  """Compares sysName of the given probes"""
  if lhs.sysName and rhs.sysName and lhs.sysName == rhs.sysName:
    return True
  return lhs == rhs
