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

XPEDITE_DIR=`dirname $0`

VENV_CMD=virtualenv

if ! type ${VENV_CMD} >/dev/null 2>&1; then
  VENV_CMD=virtualenv2
  if ! type ${VENV_CMD} >/dev/null 2>&1; then
    echo xpedite requires python virtualenv to install dependencies. Please install virtualenv and run this script again
    exit 1
  fi
fi

${VENV_CMD} ${XPEDITE_DIR}/install/runtime
if [ $? -ne 0 ]; then
  echo failed to create virtual environment ...
  exit 1
fi

RUNTIME_DIR=${XPEDITE_DIR}/install/runtime/bin

${RUNTIME_DIR}/python -m pip install -r ${XPEDITE_DIR}/scripts/lib/xpedite/requirements.txt
if [ $? -ne 0 ]; then
  echo failed to install python dependencies...
  exit 1
fi


export PATH=${RUNTIME_DIR}/bin:${PATH}

if [ ${VERBOSE} -eq 1 ]; then
  echo 'downloading jupyter dark theme ...'
fi
wget https://raw.githubusercontent.com/powerpak/jupyter-dark-theme/master/custom.css -O ${XPEDITE_DIR}/scripts/jupyter/config/custom/darkTheme.css

if [ $? -ne 0 ]; then
  echo failed to download jupyter theme...
fi

if [ ${ENABLE_PMU} -eq 1 ]; then
  if [ ${VERBOSE} -eq 1 ]; then
    echo 'downloading uarch spec and topdown metrics ...'
  fi
  #Download uarch spec database and pmu js
  $XPEDITE_DIR/scripts/bin/xpedite list >/dev/null 2>&1 
  if [ $? -ne 0 ]; then
    echo failed to install micro architecture spec files
  fi
fi
