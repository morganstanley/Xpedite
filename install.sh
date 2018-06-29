#!/usr/bin/env bash
################################################################################################
#
# Download and install juypter theme, uarch spec and topdown metrics
#
# Author: Manikandan Dhamodharan, Morgan Stanley
#
################################################################################################

usage() {
cat << EOM
Usage: $0 [OPTION]...
Install xpedite profiler.

Mandatory arguments to long options are mandatory for short options too.
  -v, --verbose                   verbose mode
  -p, --enablePMU                 downloads files to enable performance counters
EOM
exit 1
}

ARGS=`getopt -o vp --long verbose,enablePMU -- "$@"`
if [ $? -ne 0 ]; then
  usage
fi

eval set -- "$ARGS"
ENABLE_PMU=0
VERBOSE=0

while true ; do
  case "$1" in
    -p|--enablePMU)
        ENABLE_PMU=1 ; shift ;;
    -v|--verbose)
        VERBOSE=1 ; shift ;;
    --) shift ; break ;;
    *) usage ;;
  esac
done

XPEDITE_DIR=`dirname $0`/scripts

if [ ${VERBOSE} -eq 1 ]; then
  echo 'downloading jupyter dark theme ...'
fi
wget https://raw.githubusercontent.com/powerpak/jupyter-dark-theme/master/custom.css -O ${XPEDITE_DIR}/jupyter/config/custom/darkTheme.css

if [ ${ENABLE_PMU} -eq 1 ]; then
  if [ ${VERBOSE} -eq 1 ]; then
    echo 'downloading uarch spec and topdown metrics ...'
  fi
  #Download uarch spec database and pmu js
  $XPEDITE_DIR/bin/xpedite list 2>&1 >/dev/null
fi
