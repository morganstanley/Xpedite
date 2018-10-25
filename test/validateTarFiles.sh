#!/usr/bin/env bash
#############################################################################
#
# Validate archive files used by Xpedite pytests
#
# Author: Brooke Elizabeth Cantwell, Morgan Stanley
#
#############################################################################

TEST_DIR=`dirname $0`
DATA_DIR=`readlink -f ${TEST_DIR}/pytest/test_xpedite/data`

declare -a APPS=("allocatorApp" "dataTxnApp" "multiThreadedApp" "slowFixDecoderApp")

function usage() {
cat << EOM
-------------------------------------------------------------------------------------------------
usage: ${PROGRAM_NAME}
-------------------------------------------------------------------------------------------------
EOM
exit 1
}

function message() {
  echo -e "_________________________________________________________\n"
  echo $1
  echo "_________________________________________________________"
}

function validate() {
  TEMP_DIR=`mktemp -d`

  if [ "$?" -ne "0" ]; then
    "failed to create temporary directory"
    exit 1
  fi

  rm -rf ${TEMP_DIR}/*
  tar -C ${TEMP_DIR} -zxvf ${DATA_DIR}/$1.tar.gz

  MANIFEST_FILE=`readlink -f ${DATA_DIR}/$1Manifest.csv`
  MANIFEST=`tail -n +2 "${MANIFEST_FILE}"`

  if [ `tar -tf "${DATA_DIR}"/"$1".tar.gz | grep -vc "/$"` != `echo "${MANIFEST}" | wc -l` ]; then
    message "NUMBER OF FILES IN $1 ARCHIVE DOES NOT MATCH MANIFEST"
    exit 1
  fi
  
  IFS=,
  while read -r FILE SIZE LINES; do
    FULL_PATH=${TEMP_DIR}/${FILE}

    if [[ ${FULL_PATH: -4} == *.data ]]; then
      FILE_LINES=`${TEST_DIR}/../install/bin/xpediteSamplesLoader "${FULL_PATH}" | wc -l`
    else
      FILE_LINES=`wc -l < "${FULL_PATH}"`
    fi

    if [ ! -f "${FULL_PATH}" ]; then
      message "FILE ${FILE} FROM MANIFEST NOT LOCATED IN ARCHIVE"
      exit 1
    fi

    if [ `wc -c < "${FULL_PATH}"` != "${SIZE}" ]; then
      message "${FILE} SIZE DOES NOT MATCH MANIFEST"
      exit 1
    fi

    if [ "${FILE_LINES}" !=  "${LINES}" ]; then
      message "${FILE} LINE COUNT DOES NOT MATCH MANIFEST"
      exit 1
    fi

  done <<< "${MANIFEST}"

  message "$1 ARCHIVE FILE HAS BEEN VALIDATED"
}

for i in "${APPS[@]}"; do
  validate ${i}
done
