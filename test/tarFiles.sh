#!/usr/bin/env bash
#############################################################################
#
# Zip / unzip data files for pytests
#
# Author: Brooke Elizabeth Cantwell, Morgan Stanley
#
#############################################################################

TEST_DIR=`dirname $0`
PYTEST_DIR=${TEST_DIR}/pytest

source ${TEST_DIR}/.testrc

SAMPLES_LOADER=`fullPath ${TEST_DIR}/../install/bin/xpediteSamplesLoader`

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
        tar -C $1 -zxf ${PYTEST_DIR}/test_xpedite/data/${a}${s}.tar.gz
      else
        tar -C $1 -zxf ${PYTEST_DIR}/test_xpedite/data/${a}${s}.tar.gz --exclude=${a}${s}/benchmark --exclude=${a}${s}/expectedResults --exclude=${a}${s}/parameters/data
      fi
    done
  done
}

function zipFiles() {
  FILE_PATH=`fullPath ${PYTEST_DIR}`
  TEST_DIR_PATH=`fullPath ${TEST_DIR}`
  
  cd $1
  find . -name "*.pyc" -type f -delete
  
  for a in "${APPS[@]}"; do
    for s in "${SCENARIOS[@]}"; do
      MANIFEST_PATH=${TEST_DIR_PATH}/pytest/test_xpedite/data/${a}${s}Manifest.csv
      rm ${MANIFEST_PATH}

      echo "file name, size, lines" >> ${MANIFEST_PATH}
      for FILE in `find ${a}${s} -type f | sort`; do
        FULL_PATH=$1/${FILE}
        echo -n "${FILE}," >> ${MANIFEST_PATH}
        echo -n "`wc -c < ${FULL_PATH}`," >> ${MANIFEST_PATH}
        if [[ ${FULL_PATH: -5} == ".data" ]]; then
          echo "`${SAMPLES_LOADER} ${FULL_PATH} | wc -l`" >> ${MANIFEST_PATH}
        else
          echo "`wc -l < ${FULL_PATH}`" >> ${MANIFEST_PATH}
        fi
      done
      ARCHIVE_FILE=${FILE_PATH}/test_xpedite/data/${a}${s}.tar.gz
      tar -czf ${ARCHIVE_FILE} ${a}${s} --files-from=${a}${s}/parameters --files-from=${a}${s}/expectedResults
      if [ "${s}" == "Benchmark" ]; then
        tar -czf ${ARCHIVE_FILE} ${a}${s} --files-from=${a}${s}/benchmark
      fi
    done
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
      zipFiles ${DIR_NAME}
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
