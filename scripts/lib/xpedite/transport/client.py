"""
TCP client

Author: Dhruv Shekhawat, Morgan Stanley
"""

import logging
import six
import socket
from xpedite.util import parsePort
import time

LOGGER = logging.getLogger(__name__)

class Client(object):
  """
  A Basic TCP Client

  Connects to a TCP server via the standard Session Protocol.
  """

  def __init__(self, host, port, interface=None):
    """
    Create a new TCP client.
    This constructor takes parameters that specify the address (host, port) to connect to
    and an optional logging callback method.

    :param host: hostname or IP address to connect to
    :type host: str
    :param port: port to connect to
    :type port: str or int
    :param interface: Local interface to bind to. Defaults to None, in which case
      the socket does not bind before connecting. An example value might be
      (:py:func:`ets.net.interface_to_ipv4('eth0') <ets.net.interface_to_ipv4>`, 0)}.
    :type interface: (str, str or int) tuple
    """
    self.host = host
    self.port = parsePort(port)
    self.interface = interface
    self.socket = None
    self.timeout = 30

  def connect(self):
    """
    Connect client to socket.


    :returns: None
    :rtype: NoneType

    """
    LOGGER.debug('Connecting socket to %s:%s', self.host, self.port)
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if self.interface is not None:
      self.socket.bind(self.interface)
    return self.socket.connect((self.host, self.port))

  def send(self, msg):
    """
    Send the given message.

    :param msg: message to be sent
    :type msg: bytes
    :returns: Timestamp when msg sent (in microseconds from epoch) and number of bytes sent
    :rtype: tuple of long and int

    """
    LOGGER.debug('Sending msg %s', msg)

    tsp = time.time() * 1000000
    size = self.socket.send(six.ensure_binary(msg))
    return (tsp, size)

  def receive(self, size, timeout=30):
    """
    Receive a message.

    :param size: Number of bytes to receive
    :type size: int
    :param timeout: timeout in seconds (Default value = 30)
    :type timeout: int
    :returns: message received
    :rtype: bytes

    """
    if timeout != self.timeout:
      self.timeout = timeout
    self.socket.settimeout(timeout)
    try:
      msg = self.socket.recv(size)
    except Exception:
      if timeout == 0:
        raise socket.timeout
      raise

    return bytes(msg)

  def recv(self, bufsize, flags=0):
    """
    Proxy for Python's socket.recv().

    :param bufsize: Maximum amount of data to be received at once
    :type bufsize: int
    :param flags: See the Unix manual page recv(2) for the meaning of the optional
      argument flags; it defaults to zero.
    :type flags: int
    :returns: message received
    :rtype: bytes

    """
    return self.socket.recv(bufsize, flags)

  def close(self):
    """
    Close the connection.

    :returns: None
    :rtype: NoneType

    """
    if self.socket is not None:
      self.socket.close()
    LOGGER.debug('Closed socket')
