#!/usr/bin/env bash
##############################################################################################################
#
# Generate Xpedite pytest
#
# Author: Brooke Elizabeth Cantwell, Morgan Stanley
#
##############################################################################################################

TEST_DIR=`dirname $0`
PYTEST_DIR=${TEST_DIR}/pytest
XPEDITE_DIR=${TEST_DIR}/../scripts/lib

if [ ! -d ${XPEDITE_DIR} ]; then 
  echo "Failed to locate xpedite package"
  exit 1
fi

export PYTHONPATH=$PYTHONPATH:${PYTEST_DIR}
export PYTHONPATH=$PYTHONPATH:${XPEDITE_DIR}

TEMP_DIR=`mktemp -d`
APP_NAME="slowFixDecoder"

${TEST_DIR}/tarFiles.sh -d ${TEMP_DIR} -a ${APP_NAME} -x

python ${PYTEST_DIR}/test_xpedite/test_profiler/generateBaseline.py ${TEMP_DIR} ${APP_NAME}

${TEST_DIR}/tarFiles.sh -d ${TEMP_DIR} -a ${APP_NAME} -z
