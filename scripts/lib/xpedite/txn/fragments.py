"""
Logic to build transactions from multiple fragments

This module handles loading of transaction fragments from multiple threads.

The loaded fragments are linked (suspending to resuming and vice versa) to
create a chain of framgents to complete a transaction.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import copy
import logging
LOGGER = logging.getLogger(__name__)

class Key(object):
  """Key to lookup suspending and resuming transaction fragments"""

  def __init__(self, linkId, resuming):
    """
    Constructs a key for lookup of transaction fragments

    :param linkId: Identifier to link suspending fragment to resuming ones
    :param resuming: Boolean to identify resuming transactions

    """
    self.linkId = linkId
    self.resuming = resuming

  def __hash__(self):
    return hash(self.linkId) ^ hash(self.resuming)

  def __eq__(self, other):
    return self.linkId == other.linkId and self.resuming == other.resuming

  def __repr__(self):
    return '{} fragment - {}'.format('resume' if self.resuming else 'suspend', self.linkId)

class ResumeKey(Key):
  """Key to lookup resuming transaction fragments"""

  def __init__(self, linkId):
    Key.__init__(self, linkId, True)

class SuspendKey(Key):
  """Key to lookup suspending transaction fragments"""

  def __init__(self, linkId):
    Key.__init__(self, linkId, False)

class TxnFragment(object):
  """A transaction fragment to keep track of suspending and resuming transactions"""

  def __init__(self, txn, resumeId=None, suspendId=None):
    """
    Constructs an instance of transaction fragment

    :param txn: A partial transaction that was suspended or resumed

    """
    self.txn = txn
    self.resumeId = resumeId
    self.suspendId = suspendId
    self.next = None

  def __repr__(self):
    return 'txn - {} | suspending {},{} | resuming {},{}'.format(
      self.txn,
      self.suspendId[0:16] if self.suspendId else '', self.suspendId[16:] if self.suspendId else '',
      self.resumeId[0:16] if self.resumeId else '', self.resumeId[16:] if self.resumeId else '',
    )

class TxnFragments(object):
  """A collection of suspending and resuming transaction fragemnts"""

  def __init__(self):
    """Constructs an instance of transaction fragment collection"""
    self.fragments = {}
    self.rootFragments = []
    self.nextTxnId = 0

  def addResumeFragment(self, linkId, txn):
    """
    Adds a resuming transaction fragment to the collection.

    Resuming fragments lookup for the previous suspended fragement to form a link

    :param linkId: Identifier to link suspending fragment to resuming ones
    :param txn: A partial transaction that was resumed

    """
    resumeFragment = TxnFragment(txn, resumeId=linkId)
    resumeKey = ResumeKey(linkId)
    resumeFragments = self.fragments.get(resumeKey, None)
    if not resumeFragments:
      resumeFragments = []
      self.fragments.update({resumeKey:resumeFragments})
      suspendFragment = self.fragments.get(SuspendKey(linkId), None)
      if suspendFragment:
        # found the previous fragment suspending the txn
        suspendFragment.next = resumeFragments
    resumeFragments.append(resumeFragment)
    LOGGER.trace('Adding %s', resumeFragment)
    return resumeFragment

  def addSuspendFragment(self, linkId, txn, fragment):
    """
    Adds a suspending transaction fragment to the collection.

    Suspending fragments lookup for the next resuming fragement to form a link

    :param linkId: Identifier to link suspending fragment to resuming ones
    :param txn: A partial transaction that was suspended

    """
    if not fragment:
      fragment = TxnFragment(txn, suspendId=linkId)
      self.rootFragments.append(fragment)
    self.fragments.update({SuspendKey(linkId):fragment})
    resumeFragments = self.fragments.get(ResumeKey(linkId), None)
    if resumeFragments:
      # found the previous fragment suspending the txn
      fragment.next = resumeFragments
    LOGGER.trace('Adding %s', fragment)

  def joinFragments(self, txns, txn, fragments):
    """Joins fragments in a link to compose a compound transaction"""
    if not fragments:
      self.nextTxnId += 1
      txn.txnId = self.nextTxnId
      txns.append(txn)
    elif len(fragments) == 1:
      txn.join(fragments[0].txn)
      self.joinFragments(txns, txn, fragments[0].next)
    else:
      txnClone = copy.deepcopy(txn)
      for i, fragment in enumerate(fragments):
        txn.join(fragment.txn)
        self.joinFragments(txns, txn, fragment.next)
        if i+1 < len(fragments) -1:
          txn = copy.deepcopy(txnClone)
        else:
          txn = txnClone

  def join(self, nextTxnId):
    """Joins all fragments in the collection to compose a compound transactions"""
    self.nextTxnId = nextTxnId
    txns = []
    for rootFragment in self.rootFragments:
      self.joinFragments(txns, rootFragment.txn, rootFragment.next)
    return txns
