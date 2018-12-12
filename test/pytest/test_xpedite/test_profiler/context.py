"""
Module to store information about the context pytests are run in
that are used in scenarios.

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

class Context(object):
  """
  Class to store information used to run pytests
  """
  def __init__(self, txnCount, threadCount, workspace=None):
    self.txnCount = txnCount
    self.threadCount = threadCount
    self.workspace = workspace
