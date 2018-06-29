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

export PYTHONPATH=${XPEDITE_DIR}
python ${PYTEST_DIR}/test_xpedite/test_profiler/generateBaseline.py
