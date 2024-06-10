#!/usr/bin/env bash
##############################################################################################################
#
# Script to run Xpedite pytests, gtests, and pylint
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

if [ ! -d  ${PYTEST_DIR} ]; then
  echo "Failed to locate pytest scripts"
  exit 1
fi


function usage() {
cat << EOM
-------------------------------------------------------------------------------------------------

usage: ${PROGRAM_NAME} [lgpw:cr:s:Pt:m:a:]
-l|--lint           run lint test
-g|--gtest          run gtest
-p|--pytest         run pytests
-w|--workspace      workspace path to trim from the beginning of file paths
-c|--cov            check pytest code coverage
-r|--remote         set a remote hostname for the application to run on: ${PROGRAM_NAME} -r <hostname>
-s|--single         choose a single test to run: ${PROGRAM_NAME} -s test_name
-L|--list           List pytest
-k|--pattern        Run pytest matching pattern
-t|--transactions   specify a number of transactions for the target application: ${PROGRAM_NAME} -t <transactions>
-m|--multithreaded  specify the number of threads for the target application: ${PROGRAM_NAME} -m <number of threads>
-a|--apps           a comma separated list of binaries to test: ${PROGRAM_NAME} -a <app1,app2,app3>
-S|--scenarioTypes  a comma separated list of scenarios to run: ${PROGRAM_NAME} -S <Regular,Benchmark>
-P|--recordPMC      enable recording performance counters during testing

-------------------------------------------------------------------------------------------------

examples:
to run locally: ${PROGRAM_NAME}
to run only pytests: ${PROGRAM_NAME} -p
to run remotely: ${PROGRAM_NAME} -r <hostname>
to run one test: ${PROGRAM_NAME} -s test_record_vs_report
to run tone target application to create 3000 transactions with 3 threads: ${PROGRAM_NAME} -t 3000 -m 3
to run only pytests for one application: ${PROGRAM_NAME} -p <app>

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
-t|--transactions   specify a number of transactions for the target application
-m|--multithreaded  specify the number of threads for the target application
-a|--apps           a comma separated list of binaries to test
-S|--scenarioTypes  a comma separated list of scenarios to run
-P|--recordPMC      enable PMC recording during testing
-------------------------------------------------------------------------------------------------

EOM
exit 1
}

function rsyncSource() {
  SRC_DIR=${TEST_DIR}/../../../../..
  SRC_PATH=$(readlink -f ${SRC_DIR})
  DEST_DIR=${SRC_DIR}/..
  DEST_PATH=$(readlink -f ${DEST_DIR})

  if ! doesDirectoryExist $1 ${SRC_PATH}; then
    echo "_____________________________"
    echo "rsyncing to host $1 ..."
    echo "_____________________________"
    
    if ! rsync -avz ${SRC_PATH} $1:${DEST_PATH}; then
      echo "failed to rsync xpedite source files ..."
      exit 1
    fi
  fi
}

function doesDirectoryExist() {
  if ssh $1 '[ -d $2 ]'; then
    true
  else
    false
  fi
}

function listPytests() {
	PYTHONPATH=${XPEDITE_DIR}:${PYTHONPATH} pytest ${PYTEST_DIR} --collect-only
}

function runPytests() {
  RUN_DIR=$(mktemp -d)

  if [ "$?" -ne "0" ]; then
    "failed to create temporary directory"
    exit 1
  fi

  rm -rf ${RUN_DIR}/*
  ${TEST_DIR}/tarFiles.sh -d ${RUN_DIR} -x

  RUN_DIR_ARG="--rundir=${RUN_DIR}"

  if [ -z "${TEST_NAME}" ]; then
    TEST_NAME=${PYTEST_DIR}
  fi

  if [ -z "${APPS}" ]; then
    APPS="--apps=allocatorApp,dataTxnApp,multiThreadedApp,slowFixDecoderApp"
  fi

  if [ -z "${SCENARIO_TYPES}" ]; then
    SCENARIO_TYPES="--scenarioTypes=Regular,Benchmark,PMC"
  fi

  PYTEST_ARGS="${COV} ${TEST_NAME} ${TEST_PATTERN} -v ${APP_HOST} ${PYTEST_ARGS} ${TRANSACTION_COUNT} ${THREAD_COUNT}"
  PYTEST_ARGS="${PYTEST_ARGS} ${WORKSPACE} ${RUN_DIR_ARG} ${APPS} ${SCENARIO_TYPES} ${RECORD_PMC}"
  if ! PYTHONPATH=${XPEDITE_DIR}:${PYTHONPATH} FORCE_COLOR=true pytest ${PYTEST_ARGS}; then
    echo detected one or more pytest failures
    RC=$(($RC + 1))
  fi
}

function runLint() {
  
  if ! pylint --rcfile ${XPEDITE_DIR}/../.pylintrc ${XPEDITE_DIR}/xpedite; then
    echo detected pylint violations
    RC=$(($RC + 1))
  fi
  
  if ! PYTHONPATH=${XPEDITE_DIR}:${PYTHONPATH} pylint --rcfile ${XPEDITE_DIR}/../.pylintrc ${PYTEST_DIR}/test_xpedite; then
    echo detected pylint violations
    RC=$(($RC + 1))
  fi

}

function runGtests() {
  TEST_APP=${TEST_DIR}/../install/test/testXpedite
  if [ ! -x ${TEST_APP} ]; then
    echo "Failed to locate test binary (${TEST_APP}). Have you built xpedite gtests ?" > /dev/stderr
    exit 1
  fi

  FILTER=''
  if [ ! "${PMC}" ]; then
    FILTER=--gtest_filter="-PMC*Test.*"
  fi

  if ! ${TEST_APP} $FILTER; then
    echo detected failure of one or more gtests
    RC=$(($RC + 1))
  fi
}

function runAllTests() {
  runGtests
  runLint
  runPytests
}

ARGS=$(getopt -o lLgpw:cr:s:k:t:m:a:S:P --long lint,list,gtest,pytest,single,pattern,workspace,cov,remote:,test:,transactions:,multithreaded:,apps:,scenarioTypes:recordPMC -- "$@")

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
    -k|--pattern)
      TEST_PATTERN="-k $2" ;
			echo 'Test pattern *******' ${TEST_PATTERN}
      shift 2
      ;;
    -L|--list)
      LIST_PYTEST=true
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
    -a|--apps)
      APPS="--apps=$2"
      shift 2
      ;;
    -S|--scenarioTypes)
      SCENARIO_TYPES="--scenarioTypes=$2"
      shift 2
      ;;
    -P|--recordPMC)
      RECORD_PMC="--recordPMC=True"
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
RUNTIME_DIR=${TEST_DIR}/../install/runtime

if [ -d ${RUNTIME_DIR}/bin ]; then
  echo detected virtual environment. resolving python dependencies from ${RUNTIME_DIR}/bin
  export PATH=${RUNTIME_DIR}/bin:${PATH}
  python -m pip --trusted-host pypi.org --trusted-host files.pythonhosted.org install pytest pylint pytest-cov
fi

if [[ -z "${LINT}" && -z "${GTEST}" && -z "${PYTEST}" && -z "${LIST_PYTEST}" && -z "${TEST_PATTERN}" ]]; then
  runAllTests
else
  if [ -z "${PYTEST}" ] && [[ "${APP_HOST}" || "${TRANSACTION_COUNT}"  || "${THREAD_COUNT}" || "${WORKSPACE}" || "${COV}" || "${TEST_NAME}" || "${APPS}" || "${SCENARIO_TYPES}" || "${RECORD_PMC}" ]]; then
    pytestUsage
  fi

  if [ "${GTEST}" = true ]; then
    runGtests
  fi

  if [ "${LINT}" = true ]; then
    runLint
  fi

  if [ "${PYTEST}" = true -o -n "${TEST_PATTERN}" ]; then
    runPytests
  fi

  if [ "${LIST_PYTEST}" = true ]; then
		listPytests
  fi
fi

exit $RC
