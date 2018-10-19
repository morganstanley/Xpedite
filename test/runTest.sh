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

usage: ${PROGRAM_NAME} [lgpw:cr:s:Pt:m:c:]
-l|--lint           run lint test
-g|--gtest          run gtest
-p|--pytest         run pytests
-w|--workspace      workspace path to trim from the beginning of file paths
-c|--cov            check pytest code coverage
-r|--remote         set a remote hostname for the application to run on: ${PROGRAM_NAME} -r <hostname>
-s|--single         choose a single test to run: ${PROGRAM_NAME} -s test_name
-P|--pmc            use flag to enable performance counters: ${PROGRAM_NAME} -p
-t|--transactions   specify a number of transactions for the target application: ${PROGRAM_NAME} -t <transactions>
-m|--multithreaded  specify the number of threads for the target application: ${PROGRAM_NAME} -m <number of threads>

-------------------------------------------------------------------------------------------------

examples:
to run locally: ${PROGRAM_NAME}
to run remotely: ${PROGRAM_NAME} -r <hostname>
to run remotely with pmc: ${PROGRAM_NAME} -r <hostname> -P
to run one test: ${PROGRAM_NAME} -s test_record_against_report
to run the target application to create 3000 transactions with 3 threads: ${PROGRAM_NAME} -t 3000 -m 3

-------------------------------------------------------------------------------------------------

EOM
exit 1
}

function pytestUsage() {
cat << EOM
-------------------------------------------------------------------------------------------------

the following flags can only be enabled when running pytests
-c|--cov            check pytest code coverage
-r|--remote         set a remote hostname for the application to run on
-s|--single         choose a single test to run
-P|--pmc            use flag to enable performance counters
-t|--transactions   specify a number of transactions for the target application
-m|--multithreaded  specify the number of threads for the target application

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

function runPytests() {
  TEMP_DIR=`mktemp -d`
  APP_NAME='slowFixDecoder'

  ${TEST_DIR}/tarFiles.sh -d ${TEMP_DIR} -a ${APP_NAME} -x
  TEMP_DIR_ARG="--tempdir=${TEMP_DIR}"

  if [ "${PMC}" ]; then
    # run test with performance counters
    PYTHONPATH=${XPEDITE_DIR} pytest ${COV} ${TEST_NAME} -v ${APP_HOST} ${TRANSACTION_COUNT} ${THREAD_COUNT} ${WORKSPACE} ${TEMP_DIR_ARG}
  else
    # omit test with performance counters
    PYTHONPATH=${XPEDITE_DIR} pytest ${COV} ${TEST_NAME} -v ${APP_HOST} ${TRANSACTION_COUNT} ${THREAD_COUNT} ${WORKSPACE} ${TEMP_DIR_ARG} -m 'not pmc'
  fi

  if [ $? -ne 0 ]; then
    echo detected one or more pytest failures
    RC=`expr $RC + 1`
  fi
}

function runLint() {
  pylint --rcfile ${XPEDITE_DIR}/../.pylintrc ${XPEDITE_DIR}/xpedite
  if [ $? -ne 0 ]; then
    echo detected pylint violations
    RC=`expr $RC + 1`
  fi
}

function runGtests() {
  TEST_APP=${TEST_DIR}/../install/test/testXpedite
  if [ -x ${TEST_APP} ]; then
    echo 'Failed to locate demo binary. Have you built xpedite gtests ?' > /dev/stderr
  fi

  FILTER=''
  if [ ! "${PMC}" ]; then
    FILTER=--gtest_filter="-PMC*Test.*"
  fi

  ${TEST_APP} $FILTER
  if [ $? -ne 0 ]; then
    echo detected failure of one or more gtests
    RC=`expr $RC + 1`
  fi
}

function runAllTests() {
  runGtests
  runLint
  runPytests
}

TEST_NAME=${PYTEST_DIR}
COV=""

ARGS=`getopt -o lgpw:cr:s:Pt:m:c: --long lint,gtest,pytest,workspace,cov,remote:,test:,pmc,transactions:,multithreaded:,connect: -- "$@"`

if [ $? -ne 0 ]; then
  usage
fi

eval set -- "$ARGS"

while true ; do
  case "$1" in
     -l|--lint)
      LINT=true
      shift
      ;;
     -g|--gtest)
      GTEST=true
      shift
      ;;
     -p|--pytest)
      PYTEST=true
      shift
      ;;
     -w|--workspace)
      WORKSPACE="--workspace=$2"
      shift 2
      ;;
     -c|--cov)
      COV="--cov=xpedite"
      shift
      ;;
    -r|--remote)
      APP_HOST="--hostname=$2"
      rsyncSource "$2" ;
      shift 2
      ;;
    -s|--single)
      TEST_NAME="${PYTEST_DIR}/test_xpedite/test_profiler/test_profiler.py::$2" ;
      echo ${TEST_NAME}
      shift 2
      ;;
    -P|--pmc)
      PMC=true ;
      shift
      ;;
    -t|--transactions)
      TRANSACTION_COUNT="--transactions=$2"
      shift 2
      ;;
     -m|--multithreaded)
      THREAD_COUNT="--multithreaded=$2"
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
  
RC=0

if [ -d ${TEST_DIR}/../install/runtime/bin ]; then
  echo detected virtual environment. resolving python dependencies from ${TEST_DIR}/../install/runtime/bin
  export PATH=${TEST_DIR}/../install/runtime/bin:${PATH}
  python -m pip --trusted-host pypi.org --trusted-host files.pythonhosted.org install pytest pylint pytest-cov
fi

if [[ -z "${LINT}" && -z "${GTEST}" && -z "${PYTEST}" ]]; then
  runAllTests
else
  if [ "${PYTEST}" = true ] && [[ "${APP_HOST}" || "${TEST_NAME}" || "${PMC}" || "${TRANSACTION_COUNT}"  || "${THREAD_COUNT}" ]]; then
    pytestUsage
  fi

  if [ "${GTEST}" = true ]; then
    runGtests
  fi

  if [ "${LINT}" = true ]; then
    runLint
  fi

  if [ "${PYTEST}" = true ]; then
    runPytests
  fi
fi

exit $RC
