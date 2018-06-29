"""
Module to enhance auto complete of transaction attribues in jupyter shell.

Author: Manikandan Dhamodharan, Morgan Stanley
"""
import re

ILLEGAL_CHARS = re.compile('[^0-9a-zA-Z_]')
ILLEGAL_PREFIX = re.compile('^[^a-zA-Z_]+')

class Txn(object):
  """Enables auto completion of attributes for txn references in jupyter shell """

  def __init__(self, pmcNames):
    for i, pmc in enumerate(pmcNames):
      pmc = ILLEGAL_CHARS.sub('', pmc)
      pmc = ILLEGAL_PREFIX.sub('', pmc)
      setattr(self, pmc, i)
