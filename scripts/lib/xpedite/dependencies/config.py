"""
Xpedite configuration

This module provides the default values for Xpedite config parameters
The default values can be overridden using a Xpedite plugin

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os

class Config(object):
  """Xpedite config options"""

  def __init__(self, config):
    self.logDir = config.get('logDir', os.path.join('/var/tmp', os.getenv('USER'), 'xpedite'))
    self.uarchSpecPath = config.get('uarchSpecPath', '/var/tmp/xpedite')
    self.uarchSpecRepoUrl = config.get('uarchSpecRepoUrl', 'https://download.01.org/perfmon/')
    self.manifestFileName = config.get('manifestFileName', 'mapfile.csv')
    self.topdownRatiosRepoUrl = config.get('topdownRatiosRepoUrl',
      'https://raw.githubusercontent.com/andikleen/pmu-tools/93a31782131f907067339c883477075cfedb5451/'
    )
    self.sslContext = config.get('sslContext', buildDefaultContext())

  def __repr__(self):
    cfgStr = 'Xpedite Configurations'
    for k, val in vars(self).items():
      cfgStr += '\n\t{} - {}'.format(k, val)
    return cfgStr

def buildDefaultContext():
  """ Build default SSL context """
  import ssl
  ctx = ssl.create_default_context()
  ctx.check_hostname = True
  ctx.verify_mode = ssl.CERT_REQUIRED
  return ctx
