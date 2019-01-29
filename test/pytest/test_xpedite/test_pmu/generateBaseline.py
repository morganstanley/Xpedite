"""
A module to create baseline files to compare against output for Xpedite PMC commands: list,
metrics, and topdown.

Files generated:
  - metrics.txt
  - events.txt
  - topdown.txt

Files are generated for every CPU supported by Xpedite (listed in the package __init__ file
"""

import os
import argparse
from xpedite.pmu.topdown              import Topdown
from xpedite.pmu.eventsDb             import loadEventsDb
from test_xpedite.test_pmu            import (
                                        EVENTS_DB_FILE_NAME, METRICS_FILE_NAME, TOPDOWN_FILE_NAME,
                                        CPU_IDS, PMU_DATA
                                      )

def main():
  """
  Generate baseline files for each CPU supported
  """
  parser = argparse.ArgumentParser()
  parser.add_argument('--rundir', help='temporary directory where files have been unzipped')
  args = parser.parse_args()

  if not os.path.exists(os.path.join(args.rundir, PMU_DATA)):
    os.mkdir(os.path.join(args.rundir, PMU_DATA))

  for cpuId in CPU_IDS:
    eventsDb = loadEventsDb(cpuId)
    if not os.path.exists(os.path.join(args.rundir, PMU_DATA, cpuId)):
      os.mkdir(os.path.join(args.rundir, PMU_DATA, cpuId))
    topdown = Topdown(eventsDb)

    with open(os.path.join(args.rundir, PMU_DATA, cpuId, METRICS_FILE_NAME), 'w') as fileHandle:
      metrics = ['{}\n'.format(topdown.metricsToString(name)) for name in topdown.metrics()]
      fileHandle.writelines(metrics)


    with open(os.path.join(args.rundir, PMU_DATA, cpuId, TOPDOWN_FILE_NAME), 'w') as fileHandle:
      fileHandle.write(str(topdown.hierarchy))


    with open(os.path.join(args.rundir, PMU_DATA, cpuId, EVENTS_DB_FILE_NAME), 'w') as fileHandle:
      events = ['{}\n'.format(str(event)) for event in eventsDb.eventsMap.values()]
      fileHandle.writelines(events)

if __name__ == '__main__':
  main()
