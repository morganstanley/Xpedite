#!/usr/bin/env bash
##############################################################################################################
#
# Generate Xpedite pytest
#
# Author: Brooke Elizabeth Cantwell, Morgan Stanley
#
##############################################################################################################

PROGRAM_NAME=$0

TEST_DIR=$(dirname $0)
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
usage: ${PROGRAM_NAME}
-------------------------------------------------------------------------------------

EOM
exit 1
}

if [ "$#" -gt 0 ]; then
  usage
fi

RUN_DIR=$(mktemp -d)

if [ "$?" -ne "0" ]; then
  "failed to create temporary directory"
  exit 1
fi

rm -rf ${RUN_DIR}/*

${TEST_DIR}/tarFiles.sh -d ${RUN_DIR} -e -x

RUNTIME_DIR=${TEST_DIR}/../install/runtime

if [ -d ${RUNTIME_DIR}/bin ]; then
  echo detected virtual environment. resolving python dependencies from ${RUNTIME_DIR}/bin
  export PATH=${RUNTIME_DIR}/bin:${PATH}
fi

python ${PYTEST_DIR}/test_xpedite/test_profiler/generateBaseline.py --rundir ${RUN_DIR}
python ${PYTEST_DIR}/test_xpedite/test_pmu/generateBaseline.py --rundir ${RUN_DIR}

${TEST_DIR}/tarFiles.sh -d ${RUN_DIR} -z

${TEST_DIR}/validateTarFiles.sh
