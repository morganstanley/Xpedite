#!/usr/bin/env bash
#############################################################################
#
# Zip / unzip data files for pytests
#
#############################################################################

TEST_DIR=`dirname $0`
PYTEST_DIR=${TEST_DIR}/pytest

function usage() {
cat << EOM
------------------------------------------------------------------------------

usage: ${0} [xzt:a:]
-d|--dirname      directory to use for zipping / extracting
-a|--appname      app name
-x|--extract      extract files
-z|--zip          zip files

------------------------------------------------------------------------------

EOM
exit 1
}

function unzipFiles() {
  rm -rf $1/*
  tar -C $1 -zxvf ${PYTEST_DIR}/test_xpedite/data/$2.tar.gz
}

function zipFiles() {
  FILE_PATH=`readlink -f ${PYTEST_DIR}`
  cd $1
  tar --exclude='*.pyc' -czf ${FILE_PATH}/test_xpedite/data/$2.tar.gz .
}


ARGS=`getopt -o d:a:xz --long dirname:,appname:,extract,zip, -- "$@"`

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
    -a|--appname)
      APP_NAME=$2
      shift 2
      ;;
    -z|--zip)
      zipFiles ${DIR_NAME} ${APP_NAME}
      shift
      ;;
    -x|--extract)
      unzipFiles ${DIR_NAME} ${APP_NAME}
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
