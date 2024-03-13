#!/usr/bin/env bash
################################################################################################
#
# Creates a virtual environment and installs xpedite python dependencies
#
# Download and install uarch spec and topdown metrics
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
  -2, --python2                   install python 2 virtual environment
  -3, --python3                   install python 3 virtual environment
EOM
exit 1
}

ARGS=$(getopt -o vp23 --long verbose,enablePMU,python2,python3 -- "$@")
if [ $? -ne 0 ]; then
  usage
fi

eval set -- "$ARGS"
ENABLE_PMU=0
VERBOSE=0
PYTHON_VERSION=3

while true ; do
  case "$1" in
    -p|--enablePMU)
        ENABLE_PMU=1 ; shift ;;
    -2|--python2)
        PYTHON_VERSION=2 ; shift ;;
    -3|--python3)
        PYTHON_VERSION=3 ; shift ;;
    -v|--verbose)
        VERBOSE=1 ; shift ;;
    --) shift ; break ;;
    *) usage ;;
  esac
done

XPEDITE_DIR=$(dirname $0)
RUNTIME_DIR=${XPEDITE_DIR}/install/runtime

if [ ${PYTHON_VERSION} -eq 2 ]; then
  VENV_CMD=virtualenv
  if ! type ${VENV_CMD} >/dev/null 2>&1; then
    VENV_CMD=virtualenv2
    if ! type ${VENV_CMD} >/dev/null 2>&1; then
      echo xpedite requires python virtualenv to install dependencies. Please install virtualenv and run this script again
      exit 1
    fi
  fi
  VENV_CMD="${VENV_CMD} -p /usr/bin/python2.7"
elif [ ${PYTHON_VERSION} -eq 3 ]; then
  VENV_CMD='python3 -m venv'
  if ! python3 -V >/dev/null 2>&1; then
    echo xpedite cannot find python3 runtime. Please install python3 and run this script again
    exit 1
  fi
else
  echo xpedite unknown python version - runtime ${PYTHON_VERSION} not supported.
  exit 1
fi

if ! ${VENV_CMD} ${XPEDITE_DIR}/install/runtime; then
  echo failed to create virtual environment ...
  exit 1
fi

REQUIREMENTS=${XPEDITE_DIR}/scripts/lib/xpedite/requirements.txt
if ! ${RUNTIME_DIR}/bin/python -m pip --trusted-host pypi.org --trusted-host files.pythonhosted.org install -r ${REQUIREMENTS}; then
  echo failed to install python dependencies...
  exit 1
fi

export PATH=${RUNTIME_DIR}/bin:${PATH}

if [ ${ENABLE_PMU} -eq 1 ]; then
  if [ ${VERBOSE} -eq 1 ]; then
    echo 'downloading uarch spec and topdown metrics ...'
  fi
  #Download uarch spec database and pmu js
  if ! ${RUNTIME_DIR}/python $XPEDITE_DIR/scripts/bin/xpedite list >/dev/null 2>&1; then
    echo failed to install micro architecture spec files
  fi
fi
