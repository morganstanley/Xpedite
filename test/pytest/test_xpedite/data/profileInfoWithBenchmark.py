#################################################################################
##
## Xpedite profile information file
##
#################################################################################

import os
import sys
sys.path.append(os.path.dirname(__file__))
from profileInfo import *

dataDir = os.path.dirname(__file__)
benchmarkDir = os.path.join(dataDir, 'benchmark') 
benchmarkPaths = [ 
  benchmarkDir,
]
