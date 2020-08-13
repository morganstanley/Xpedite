#!/usr/bin/env bash
################################################################################################
#
# Xpedite demo
#
# The script starts the demo application in the background
# and attaches a profiler for collecting profile data and genearating a report.
#
# The profiler's heartbeat interval is set to 1 second, to detect and end profiling, when
# the application terminates.
#
# Author: Manikandan Dhamodharan, Morgan Stanley
#
################################################################################################

usage() {
cat << EOM
Usage: $0 [OPTION]... -- [XPEDITE OPTION]...
Run a demo for xpedite profiling.

Mandatory arguments to long options are mandatory for short options too.
  -r, --remote               remote host where the app runs.
  -c, --cpu                  pin to the given cpu core
  -p, --pmc                  collect hardware performance counters
EOM
exit 1
}

ARGS=$(getopt -o r:c:p --long remote:,cpu:,pmc -- "$@")
if [ $? -ne 0 ]; then
  usage
fi

APP_LAUNCHER='eval'
eval set -- "$ARGS"
CPU=0

while true ; do
  case "$1" in
    -r|--remote)
        APP_HOST=$2
        APP_LAUNCHER="ssh $APP_HOST" ; shift 2 ;;
    -c|--cpu)
        CPU=$2 ; shift 2 ;;
    -p|--pmc)
        PMC=true; shift 1 ;;
    --) shift ; break ;;
    *) usage ;;
  esac
done

DEMO_DIR=$(dirname $0)
DEMO_DIR=$(readlink -f ${DEMO_DIR})

PROFILER=${DEMO_DIR}/../scripts/bin/xpedite
if [ ! -x ${PROFILER} ]; then
  echo 'Fatal error - failed to locate xpedite profiler.' > /dev/stderr
  exit 1
fi

DEMO_APP=${DEMO_DIR}/../install/bin/xpediteDemo
if [ ! -x ${DEMO_APP} ]; then
  echo 'Failed to locate demo binary. Have you built xpedite ?' > /dev/stderr
  exit 1
fi

if [ -d ${DEMO_DIR}/../install/runtime/bin ]; then
  echo detected virtual environment. resolving python dependencies from ${DEMO_DIR}/../install/runtime
  export PATH=${DEMO_DIR}/../install/runtime/bin:${PATH}
fi

LOG_DIR=$(${APP_LAUNCHER} "mktemp -d")
echo Xpedite demo log dir - $LOG_DIR
if [ ! -z ${APP_HOST} ]; then
  rsync -a ${DEMO_APP} ${APP_HOST}:${LOG_DIR}/
else
  ln -s ${DEMO_APP} ${LOG_DIR}/
fi

APP_NAME=$(basename ${DEMO_APP})
DEMO_APP_PID=$(${APP_LAUNCHER} "cd $LOG_DIR; ./${APP_NAME} -c ${CPU} >$LOG_DIR/app.log 2>&1& echo \\\$!")

XPEDITE_DEMO_APP_HOST=${APP_HOST:-localhost} XPEDITE_DEMO_LOG_DIR=${LOG_DIR} XPEDITE_DEMO_PMC=${PMC} XPEDITE_DEMO_CPU_SET=${CPU} ${PROFILER} record -p ${DEMO_DIR}/profileInfo.py -H 1 "$@"

if [ ! -z ${DEMO_APP_PID} ]; then
  ${APP_LAUNCHER} "kill ${DEMO_APP_PID}"
fi
wait
