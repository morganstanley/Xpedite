#!/usr/bin/env bash
##############################################################################################################
#
# Generate Xpedite pytest
#
# Author: Brooke Elizabeth Cantwell, Morgan Stanley
#
##############################################################################################################

PROGRAM_NAME=$0

TEST_DIR=`dirname $0`
PYTEST_DIR=${TEST_DIR}/pytest
XPEDITE_DIR=${TEST_DIR}/../scripts/lib

if [ ! -d ${XPEDITE_DIR} ]; then 
  echo "Failed to locate xpedite package"
  exit 1
fi

export PYTHONPATH=$PYTHONPATH:${PYTEST_DIR}
export PYTHONPATH=$PYTHONPATH:${XPEDITE_DIR}

function usage() {
cat << EOM
-------------------------------------------------------------------------------------
usage: ${PROGRAM_NAME} -h
-h|--hostname   remote host to generate files on (host must have performance counters enabled
-------------------------------------------------------------------------------------

EOM
exit 1
}

ARGS=`getopt -o h: --long hostname: -- "$@"`

if [ $? -ne 0 ]; then
  usage
fi

eval set -- "$ARGS"

while true; do
  case "$1" in
    -h|--hostname)
      HOST_NAME=$2
      shift 2
      ;;
    --)
      shift ;
      break
      ;;
    *)
      usage
      ;;
  esac
done

RUN_DIR=`mktemp -d`

if [ "$?" -ne "0" ]; then
  "failed to create temporary directory"
  exit 1
fi

rm -rf ${RUN_DIR}/*

${TEST_DIR}/tarFiles.sh -d ${RUN_DIR} -e -x

python ${PYTEST_DIR}/test_xpedite/test_profiler/generateBaseline.py --rundir ${RUN_DIR} --hostname ${HOST_NAME}

${TEST_DIR}/tarFiles.sh -d ${RUN_DIR} -z

${TEST_DIR}/validateTarFiles.sh
