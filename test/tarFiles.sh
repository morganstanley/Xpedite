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
SAMPLES_LOADER=`readlink ${TESTDIR}/../install/bin/xpediteSamplesLoader`

function usage() {
cat << EOM
------------------------------------------------------------------------------

usage: $0 [xzt:a:]
-d|--dirname      directory to use for zipping / extracting
-a|--appname      app name
-x|--extract      extract files
-z|--zip          zip files

------------------------------------------------------------------------------

EOM
exit 1
}

declare -a APPS=("allocatorApp" "dataTxnApp" "multiThreadedApp" "slowFixDecoderApp")

function unzipFiles() {
  rm -rf $1/*

  for i in "${APPS[@]}"; do
    tar -C $1 -zxvf ${PYTEST_DIR}/test_xpedite/data/$i.tar.gz
  done
}

function zipFiles() {
  FILE_PATH=`readlink -f ${PYTEST_DIR}`
  TEST_DIR_PATH=`readlink -f ${TEST_DIR}`
  
  cd $1
  find . -name "*.pyc" -type f -delete
  for i in "${APPS[@]}"; do
    MANIFEST_PATH=${TEST_DIR_PATH}/pytest/test_xpedite/data/${i}Manifest.csv
    rm ${MANIFEST_PATH}
    
    echo "file name, size, lines" >> ${MANIFEST_PATH}
    for FILE in `find $i -type f`; do
      FULL_PATH=$1/${FILE}
      echo -n "${FILE}," >> ${MANIFEST_PATH}
      echo -n "`wc -c < ${FULL_PATH}`," >> ${MANIFEST_PATH}
      
      if [[ ${FILE: -4} == "*.data" ]]; then
        echo "`${SAMPLES_LOADER} ${FILES} | wc -l`" >> ${MANIFEST_PATH}
      else
        echo "`wc -l < ${FULL_PATH}`" >> ${MANIFEST_PATH}
      fi
    done

    tar -czf ${FILE_PATH}/test_xpedite/data/$i.tar.gz $i
  done
}

ARGS=`getopt -o d:xz --long dirname:,extract,zip -- "$@"`

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
    -x|--extract)
      unzipFiles ${DIR_NAME}
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
