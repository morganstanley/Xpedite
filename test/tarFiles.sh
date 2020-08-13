#!/usr/bin/env bash
#############################################################################
#
# Zip / unzip data files for pytests
#
# Author: Brooke Elizabeth Cantwell, Morgan Stanley
#
#############################################################################

TEST_DIR=$(dirname $0)
PYTEST_DIR=${TEST_DIR}/pytest
PY_VERSION=$(python -c 'import sys; print(sys.version_info[:][0])')
DATA_DIR=test_xpedite/dataPy${PY_VERSION}

source ${TEST_DIR}/.testrc

SAMPLES_LOADER=$(fullPath ${TEST_DIR}/../install/bin/xpediteSamplesLoader)

function usage() {
cat << EOM
------------------------------------------------------------------------------

usage: $0 [d:exz]
-d|--dirname           directory to use for zipping / extracting
-e|--excludeResults    disable unzipping for benchmarks (during baseline file generation)
-x|--extract           extract files
-z|--zip               zip files

------------------------------------------------------------------------------

EOM
exit 1
}

function unzipFiles() {
  rm -rf $1/*
  for a in "${APPS[@]}"; do
    for s in "${SCENARIOS[@]}"; do
      if [ "${EXCLUDE_RESULTS}" == false ]; then
        tar -C $1 -zxf ${PYTEST_DIR}/${DATA_DIR}/${a}${s}.tar.gz
      else
        tar -C $1 -zxf ${PYTEST_DIR}/${DATA_DIR}/${a}${s}.tar.gz --exclude=${a}${s}/benchmark --exclude=${a}${s}/expectedResults --exclude=${a}${s}/parameters/data
      fi
    done
  done
  tar -C $1 -zxf ${PYTEST_DIR}/${DATA_DIR}/${PMU_DATA}.tar.gz
}

function zipProfilerFiles() {
  FILE_PATH=$(fullPath ${PYTEST_DIR})
  TEST_DIR_PATH=$(fullPath ${TEST_DIR})
  find $1 -name "*.pyc" -type f -delete
  for a in "${APPS[@]}"; do
    for s in "${SCENARIOS[@]}"; do
      createManifest $1 ${a}${s}
      ARCHIVE_FILE=${FILE_PATH}/${DATA_DIR}/${a}${s}.tar.gz
      tar -czf ${ARCHIVE_FILE} ${a}${s} --files-from=${a}${s}/parameters --files-from=${a}${s}/expectedResults
      if [ "${s}" == "Benchmark" ]; then
        tar -czf ${ARCHIVE_FILE} ${a}${s} --files-from=${a}${s}/benchmark
      fi
    done
  done
}

function zipPMUFiles() {
  cd $1
  find . -name "*.pyc" -type f -delete
  tar -czf ${TEST_DIR_PATH}/pytest/${DATA_DIR}/${PMU_DATA}.tar.gz ${PMU_DATA}
  createManifest $1 ${PMU_DATA}
}

function createManifest() {
  cd $1
  MANIFEST_PATH=${TEST_DIR_PATH}/pytest/${DATA_DIR}/${2}Manifest.csv
  rm ${MANIFEST_PATH}
  echo "file name, size, lines" >> ${MANIFEST_PATH}
  for FILE in $(find ${2} -type f | sort); do
    FULL_PATH=$1/${FILE}
    echo -n "${FILE}," >> ${MANIFEST_PATH}
    echo -n "$(wc -c < ${FULL_PATH})," >> ${MANIFEST_PATH}
    if [[ ${FULL_PATH: -5} == ".data" ]]; then
      echo "$(${SAMPLES_LOADER} ${FULL_PATH} | wc -l)" >> ${MANIFEST_PATH}
    else
      echo "$(wc -l < ${FULL_PATH})" >> ${MANIFEST_PATH}
    fi
  done
}

ARGS=`getopt -o d:exz --long dirname:,excludeResults,extract,zip -- "$@"`
EXCLUDE_RESULTS=false

if [ $? -ne 0 ]; then
  usage
fi

eval set -- "$ARGS"

while true ; do
  case "$1" in
    -d|--dirname)
      DIR_NAME=$2
      shift 2
      ;;
    -z|--zip)
      zipProfilerFiles ${DIR_NAME}
      zipPMUFiles ${DIR_NAME}
      shift
      ;;
    -e|--excludeResults)
      EXCLUDE_RESULTS=true
      shift 
      ;;
    -x|--extract)
      unzipFiles ${DIR_NAME} ${EXCLUDE_RESULTS}
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
