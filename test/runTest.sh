#!/usr/bin/env bash
##############################################################################################################
#
# Xpedite pytest
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

if [ ! -d  ${PYTEST_DIR} ]; then
  echo "Failed to locate pytest scripts"
  exit 1
fi


function usage() {
cat << EOM
-------------------------------------------------------------------------------------------------

usage: ${PROGRAM_NAME} [-rtp]
-r set a remote hostname for the application to run on: ${PROGRAM_NAME} -r <hostname>
-t choose a single test to run: ${PROGRAM_NAME} -t path/to/file.py::Class::test_name
-p use flag to enable performance counters: ${PROGRAM_NAME} -p

-------------------------------------------------------------------------------------------------

examples:
to run locally: ${PROGRAM_NAME}
to run remotely: ${PROGRAM_NAME} -r <hostname>
to run remotely with pmc: ${PROGRAM_NAME} -r <hostname> -p
to run one test: ${PROGRAM_NAME} -t pytest/test_xpedite/test_profiler.py::test_record_against_report

-------------------------------------------------------------------------------------------------

EOM
exit 1
}

function rsyncSource() {
  SRC_DIR=${TEST_DIR}/../../../../..
  SRC_PATH=`readlink -f ${SRC_DIR}`
  DEST_DIR=${SRC_DIR}/..
  DEST_PATH=`readlink -f ${DEST_DIR}`

  if ! doesDirectoryExist $1 ${SRC_PATH}; then
    echo "rsyncing to host $1 ..."
    rsync -avz ${SRC_PATH} $1:${DEST_PATH}
  fi
}

function doesDirectoryExist() {
  if ssh $1 '[ -d $2 ]'; then
    true
  else
    false
  fi
}

TEST_NAME=${PYTEST_DIR}

ARGS=`getopt -o r:t:p --long remote:,test:,pmc -- "$@"`

if [ $? -ne 0 ]; then
  usage
fi

eval set -- "$ARGS"

while true ; do
  case "$1" in
    -r|--remote)
      APP_HOST_FLAG="--hostname=$2"
      rsyncSource "$2" ;
      shift 2
      ;;
    -t|--test)
      TEST_NAME="$2" ;
      shift 2
      ;;
    -p|--pmc)
      PMC=true ;
      shift
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

RC=0

TEST_APP=${TEST_DIR}/../install/test/testXpedite
if [ -x ${TEST_APP} ]; then
  echo 'Failed to locate demo binary. Have you built xpedite gtests ?' > /dev/stderr

  FILTER=''
  if [ ! "${PMC}" ]; then
    FILTER=--gtest_filter="-PMC*Test.*"
  fi

  ${TEST_APP} $FILTER
  if [ $? -ne 0 ]; then
    echo detected failure of one or more gtests
    RC=`expr $RC + 1`
  fi
fi

if [ -d ${TEST_DIR}/../install/runtime/bin ]; then
  echo detected virtual environment. resolving python dependencies from ${TEST_DIR}/../install/runtime/bin
  export PATH=${TEST_DIR}/../install/runtime/bin:${PATH}
  python -m pip install pytest pylint
fi

pylint --rcfile ${XPEDITE_DIR}/../.pylintrc ${XPEDITE_DIR}/xpedite
if [ $? -ne 0 ]; then
  echo detected pylint violations
  RC=`expr $RC + 1`
fi

if [ "${PMC}" ]; then
  # run test with performance counters
  PYTHONPATH=${XPEDITE_DIR} pytest ${TEST_NAME} -v ${APP_HOST_FLAG}
else
  # omit test with performance counters
  PYTHONPATH=${XPEDITE_DIR} pytest ${TEST_NAME} -v ${APP_HOST_FLAG} -m 'not pmc'
fi

if [ $? -ne 0 ]; then
  echo detected one or more pytest failures
  RC=`expr $RC + 1`
fi

exit $RC
