"""
Compare two objects, represented as dictionaries and output the
difference for fields that are added, removed or modified

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

from __future__ import print_function

def findDiff(dict1, dict2, path=""):
  """
  Recursively compare key / value pairs for two dictionaries
  """
  for key in dict1.keys():
    if not dict2.has_key(key):
      print("{}:".format(path))
      print("{} as key not in dict2\n".format(key))
    else:
      if isinstance(dict1[key], dict):
        if path == "":
          path = key
        else:
          path = path + "->" + key
        findDiff(dict1[key], dict2[key], path)
      else:
        if dict1[key] != dict2[key]:
          print("{}:".format(path))
          print(" - {} : ".format(dict1[key]))
          print(" + {} : ".format(dict2[key]))
