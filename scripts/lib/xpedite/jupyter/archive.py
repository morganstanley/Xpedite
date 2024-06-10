"""
This module provides functionality to bundle and un-bundle Xpedite profile data.
Inflator - given a ipynb file, locates the corresponding data file and bundles them into a tar archive.
Defaltor - untars an archive to extract ipynb and data files

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import os
import tarfile
import logging
from xpedite.jupyter  import DATA_DIR, DATA_FILE_EXT, ARCHIVE_FILE_EXT, EXPORT_PREFIX, NOTEBOOK_EXT

LOGGER = logging.getLogger(__name__)

class Inflator(object):
  """Inflator to build an Xpedite archive from notebook and data file"""

  def __init__(self, notebookPath):
    self.notebookPath = notebookPath
    self.homeDir = os.path.dirname(notebookPath)
    self.notebookName = os.path.basename(self.notebookPath)
    self.dataFileName = self.notebookName.replace(NOTEBOOK_EXT, DATA_FILE_EXT)
    self.dataFilePath = os.path.join(os.path.join(self.homeDir, DATA_DIR), self.dataFileName)
    self.archivePath = self.notebookPath.replace(NOTEBOOK_EXT, ARCHIVE_FILE_EXT)
    self.archive = None
    if os.path.isfile(self.archivePath):
      msg = 'Archive file {} already exists'.format(self.archivePath)
      LOGGER.error(msg)
      raise Exception(msg)

  def open(self):
    """Open Xpedite archvie file"""
    #pylint: disable=consider-using-with
    self.archive = tarfile.open(self.archivePath, 'w')

  def __enter__(self):
    self.open()
    return self

  def close(self):
    """Close Xpedite archvie file"""
    self.archive.close()

  def __exit__(self, excType, excVal, excTb):
    self.close()

  def inflate(self):
    """Build an Xpedite tar archive from notebook and data file"""
    if not os.path.isfile(self.notebookPath):
      msg = 'Notebook file {} does not exist'.format(self.notebookPath)
      LOGGER.error(msg)
      raise Exception(msg)
    if not os.path.isfile(self.dataFilePath):
      msg = '{} data file does not exist'.format(DATA_FILE_EXT)
      LOGGER.error(msg)
      raise Exception(msg)

    notebookArcname = os.path.join('', self.notebookName)
    dataFileArcname = os.path.join(DATA_DIR, self.dataFileName)
    self.archive.add(self.notebookPath, arcname=notebookArcname)
    self.archive.add(self.dataFilePath, arcname=dataFileArcname)

class Deflator(object):
  """Deflator to extract notebook and data file from Xpedite archive"""

  def __init__(self, archivePath):
    self.archivePath = archivePath
    self.archive = None

  def open(self):
    """Open Xpedite archvie file"""
    #pylint: disable=consider-using-with
    self.archive = tarfile.open(self.archivePath)

  def __enter__(self):
    self.open()
    return self

  def close(self):
    """Close Xpedite archvie file"""
    self.archive.close()

  def __exit__(self, excType, excVal, excTb):
    self.archive.close()

  def deflate(self, extractPath):
    """
    Deflate contents of Xpedtie tar archive

    :param extractPath: Path to deflate to

    """
    if not extractPath:
      import tempfile
      LOGGER.warning('No directory specified for extracting files, setting /tmp as notebook directory')
      extractPath = tempfile.mkdtemp(prefix=EXPORT_PREFIX, dir='/tmp')
    self.archive.extractall(path=extractPath)
    return extractPath
