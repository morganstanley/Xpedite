# Configuration file for jupyter-notebook.

# Add serverextension to sys.path for jupyter to load tornadoExtension module on startup
import os, sys
currDir = os.path.dirname(__file__)
packagePath = os.path.join(currDir, '../config/serverextensions')
sys.path.insert(0, packagePath)
c = get_config()
c.NotebookApp.server_extensions = [
	'tornadoExtension'
]

# Extra paths to search for static files.
# This allows adding javascript/css to be available from the notebook server
# machine, or overriding individual files in the IPython
c.NotebookApp.extra_static_paths = [os.path.join(currDir, '../js')]
