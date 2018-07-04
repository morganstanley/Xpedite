"""
Interface and default implementation for transaction classification.

This module provides templates for classifying transaction to create sub-categories
By default, all transactions are grouped together under category named 'Transaction'
However custom classifiers can be used to,  classify transaction based on arbitrary criteria.

A category is just a string, that gets tagged on a transaction.
Classifiers are invoked for each transaction prior to profile and report generation.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

class DefaultClassifier(object):

  """Classifies all transactions to one default category"""

  @staticmethod
  def classify(_):
    """
    Default implementation to map all transactions to one category

    :param _: Ignores parameter

    """
    return 'Transaction'

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

class ProbeDataClassifier(object):

  """Classifies transactions based on data in a given probe"""

  def __init__(self, probe, typeMapper):
    """
    Constructs an instance of ProbeDataClassifier

    :param probe: A probe that is expected to be in the transaction
    :type probe: xpedite.types.probe.Probe
    :param typeMapper: a callback to map probe data to a category
    :type typeMapper: bool
    """
    if not (probe and typeMapper):
      raise Exception('Argument exception - request classifier must have valid probe and type mapper')

    self.probe = probe
    self.typeMapper = typeMapper

  def classify(self, txn):
    """
    Extracts data and from a given probe and invokes callback to get transaction category

    :param txn: Transaction to be classified

    """
    counter = txn.getCounterForProbe(self.probe)
    if counter:
      data = counter.data
      if self.typeMapper:
        return self.typeMapper(data)
    else:
      return self.typeMapper(None)
    return 'Transaction'
