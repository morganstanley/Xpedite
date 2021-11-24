"""
Module to read and write xpedite binary data file

This module provides classes XpediteDataReader and XpediteDataFactory to
read from and write to binary data file respectively.
  1. XpediteDataReader - Used to extract records from data file.
  2. XpediteDataFactory - Used for writing serialized profile objects
       and compressed html reports to data file.

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import struct
from ctypes import create_string_buffer
from six.moves import cPickle as pickle

class LayoutEntry(object):
  """Stores meta data about the layout of the data file"""

  def __init__(self, offset, isMarshalled, size):
    self.offset = offset
    self.isMarshalled = isMarshalled
    self.size = size

class Record(object):
  """Stores data in binary format"""

  def __init__(self, key, description, data):
    self.key = key
    self.description = description
    self.data = data
    self.binData = None

class XpediteDataReader(object):
  """Reader to decode binary records from xpedite data file"""

  def __init__(self, dataFile):
    self.dataFile = dataFile
    self.binFile = None
    self.layout = None

  def openFile(self):
    """Opens a xpedite data file"""
    #pylint: disable=consider-using-with
    self.binFile = open(self.dataFile, 'rb')

  def closeFile(self):
    """Closes a xpedite data file"""
    self.binFile.close()

  def __enter__(self):
    self.openFile()
    self.layout = self.loadLayout()
    return self

  def __exit__(self, excType, excVal, excTb):
    self.closeFile()

  def loadLayout(self):
    """Loads the layout of records in a xpedite data file"""
    binBuffer = create_string_buffer(8)
    binBuffer = self.binFile.read(8)
    data = struct.unpack_from('i', binBuffer, offset=0)
    tableSize = data[0]

    self.binFile.seek(8)
    data = self.binFile.read(tableSize)
    layout = pickle.loads(data)
    for entry in layout.values():
      entry.offset += (tableSize + 8)
    return layout

  def getData(self, targetKey):
    """Returns data for the given key"""
    layoutEntry = self.layout[targetKey]
    self.binFile.seek(layoutEntry.offset)
    data = self.binFile.read(layoutEntry.size)

    if layoutEntry.isMarshalled:
      return pickle.loads(data)
    recordData = data
    return recordData

class XpediteDataFactory(object):
  """Factory to encode binary records to a xpedite data file"""

  def __init__(self, dataFile):
    self.dataFile = dataFile
    self.dataTable = {}

  def appendRecord(self, key, description, data):
    """Appends the given data to an internal buffer"""
    if key in self.dataTable:
      errMsg = 'failed to commit record to data file - duplicate key "{}" detected'.format(key)
      raise ValueError(errMsg)
    self.dataTable[key] = Record(key, description, data)

  def commit(self):
    """Commits accumulated data to the xpedite data file"""
    offset = 0
    layout = {}

    for key in self.dataTable:
      if not isinstance(self.dataTable[key].data, str):
        isMarshalled = True
        self.dataTable[key].binData = pickle.dumps(self.dataTable[key].data, pickle.HIGHEST_PROTOCOL)
      else:
        isMarshalled = False
        self.dataTable[key].binData = self.dataTable[key].data

      dataSize = len(self.dataTable[key].binData)
      layout[key] = LayoutEntry(offset, isMarshalled, dataSize)
      offset += dataSize

    with open(self.dataFile, 'wb') as binFile:
      pTable = pickle.dumps(layout, pickle.HIGHEST_PROTOCOL)
      pTableSize = len(pTable)

      #convert to bytes
      binBuffer = create_string_buffer(8)
      struct.pack_into('i', binBuffer, 0, pTableSize)
      binFile.write(binBuffer)
      binFile.write(pTable)

      for key in self.dataTable:
        binFile.write(self.dataTable[key].binData)
