"""
Compare two objects, represented as dictionaries and output the
difference for fields that are added, removed or modified

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

from test_xpedite.test_profiler.formatters import Formatters
from logger                                import LOG_CONFIG_PATH
import logging
import logging.config

logging.config.fileConfig(LOG_CONFIG_PATH)
LOGGER = logging.getLogger('xpedite')

def findDiff(dict1, dict2, path=''):
  """
  Recursively compare key / value pairs for two dictionaries
  """
  if dict1 == dict2:
    return

  for key in dict1.keys():
    if key not in dict2:
      LOGGER.info('---------------- DIFF ----------------\n')
      LOGGER.info('Object 1 key %s does not eixst in Object 2', key)
      continue
    if dict1[key] == dict2[key]:
      continue

    formatKey = dict1[key].__class__.__name__
    if path == '':
      path = key
    elif formatKey in Formatters.formatters:
      func = Formatters.formatters[formatKey]
      path = '{} -> {}: {}'.format(path, key, func(dict1[key]))
    else:
      path += ' -> ' + key

    if isinstance(dict1[key], dict) and isinstance(dict2[key], dict):
      findDiff(dict1[key], dict2[key], path)
    else:
      if not isinstance(dict1[key], list) or not isinstance(dict2[key], list):
        try:
          findDiff(dict1[key].__dict__, dict2[key].__dict__, path)
        except AttributeError:
          LOGGER.info('---------------- DIFF ----------------\n')
          LOGGER.info('Path: %s', path)
          LOGGER.info('Diff for Key: %s', key)
          LOGGER.info('\tObject 1 value: %s\n\tObject 2 value: %s\n', str(dict1[key]), str(dict2[key]))
          return
        return
      for i, _ in enumerate(dict1[key]):
        try:
          if dict1[key][i] != dict2[key][i]:
            findDiff(dict1[key][i].__dict__, dict2[key][i].__dict__, path + '[{}]'.format(i))
        except AttributeError:
          path = path + '[{}]'.format(i)
          if dict1[key][i] != dict2[key][i]:
            LOGGER.info(path)
            return
