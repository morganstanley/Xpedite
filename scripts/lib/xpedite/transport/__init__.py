"""
Transport package for network connectivity

This package provide modules to
  1. Establish tcp connection with target application
  2. Support running python code in a remote box using ssh and rpyc
  3. Frame a stream of data to construct datagrams
  4. Accumulator to buffer datagrams, to send in one shot

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
import sys
import socket
import logging
from xpedite.util                 import promptUser
from xpedite.transport.client     import Client
from xpedite.dependencies         import Package, DEPENDENCY_LOADER
DEPENDENCY_LOADER.load(Package.Netifaces, Package.Rpyc, Package.Six)

LOGGER = logging.getLogger(__name__)

def encode(msg):
  """
  Encodes length prefixed message

  :param msg: Message to be encoded

  """
  payload = '{0:0>8}'.format(len(msg)) + msg
  return payload

HDR_LEN = 8

class MsgAccumulator(object):
  """
  Accumulates messages for rapid publishing

  Used to create spike in transport activity
  """

  def __init__(self):
    self.reset()

  def accumulate(self, payload):
    """
    Accumulates payload by appending to a buffer

    :param payload: payload to accumulate

    """
    self.msgCount += 1
    if self.msgCount == 1:
      LOGGER.debug('\n')
    LOGGER.debug('buffering message %(#:,)d    \r', self.msgCount)
    self.buffer += payload

  def reset(self):
    """Resets the state of accumulator"""
    self.buffer = ''
    self.msgCount = 0

class Transport(object):
  """Transport abstraction with support for accumulating outgoing messages"""

  def __init__(self):
    self.accumulator = None

  def enableBuffering(self):
    """Enables accumulation of outgoing messages"""
    self.accumulator = MsgAccumulator()

def readAtleast(transport, length, timeout):
  """
  Awaits reciept of at least length bytes from underlying transport, till timeout

  :param transport: Handle to a stream based transport
  :param timeout: Max amount to time to await for incoming data
  :param length: Length of data to read

  """
  import six
  LOGGER.debug('Awaiting data %d bytes', length)
  data = ''
  while len(data) < length:
    bufLen = length - len(data)
    block = transport.receive(bufLen, timeout)
    block = six.ensure_str(block)
    if block:
      data = data + block
    else:
      raise Exception('socket closed - failed to read datagram')

  logData = data if length < 400 else '{} ...'.format(data[0:45])
  LOGGER.debug('Received data |%s|', logData)
  return data

class DatagramClient(Client, Transport):
  """Datagram client to frame messages from a stream based transport"""

  def __init__(self, host, port):
    Client.__init__(self, host, port)
    Transport.__init__(self)

  def connect(self):
    """Establishes connection to the remote end"""
    Client.connect(self)
    self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)

  def send(self, msg):
    """
    Transmits a message using the underlying transport

    :param msg: Message to be sent

    """
    payload = encode(msg)
    if self.accumulator:
      self.accumulator.accumulate(payload)
    else:
      Client.send(self, payload)

  def flush(self):
    """Flushes any accumuated outgoing messages"""
    if self.accumulator:
      LOGGER.debug('\ndispatching %s requests in one shot %(:,)d bytes', self.accumulator.msgCount,
        len(self.accumulator.buffer)
      )
      Client.send(self, self.accumulator.buffer)
      self.accumulator.reset()
    else:
      raise Exception('failed to flush - buferring not enabled')

  def readFrame(self, timeout=60):
    """
    Frames and reads a datagram from an underlying stream based transport

    :param timeout: Max amount to time to await for incoming data (Default value = 60)

    """
    data = readAtleast(self, HDR_LEN, timeout)
    if len(data) != HDR_LEN:
      errMsg = 'Failed to receive frame length ({} bytes) from underlying socket - instead recieved {} bytes'.format(
        HDR_LEN, len(data)
      )
      LOGGER.error(errMsg)
      raise Exception(errMsg)

    frameLen = int(data)
    if frameLen > 0:
      return readAtleast(self, frameLen, timeout)
    return ''
