# Configuration file for jupyter-notebook.

# Add serverextension to sys.path for jupyter to load tornadoExtension module on startup
import os
import sys

currDir = os.path.dirname(__file__)
packagePath = os.path.join(currDir, '../config/serverextensions')
sys.path.insert(0, packagePath)
c = get_config()
c.ServerApp.jpserver_extensions = {
    'tornadoExtension' : True,
}

staticPath = os.path.join(currDir, '../config/custom')
c.ServerApp.extra_static_paths = [os.path.join(currDir, '../js'), staticPath]
c.ServerApp.allow_origin = '*'  #allow all origins
c.ServerApp.ip = '0.0.0.0'      # listen on all IPs
