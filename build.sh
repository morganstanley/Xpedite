#!/usr/bin/env bash
################################################################################################
#
# Invoke Cmake to build xpedite static libarary and kernel module
#
# Author: Manikandan Dhamodharan, Morgan Stanley
#
################################################################################################

usage() {
cat << EOM
Usage: $0 [OPTION]...
Build xpedite profiler.

Mandatory arguments to long options are mandatory for short options too.
  -t, --type                      type of the build DEBUG, RELEASE
  -v, --verbose                   collect hardware performance counters
EOM
exit 1
}

ARGS=`getopt -o t:v --long type:,verbose -- "$@"`
if [ $? -ne 0 ]; then
  usage
fi

eval set -- "$ARGS"
BUILD_TYPE=Release
VERBOSE=0

while true ; do
  case "$1" in
    -t|--type)
        BUILD_TYPE=$2 ; shift 2 ;;
    -v|--verbose)
        VERBOSE=1 ; shift ;;
    --) shift ; break ;;
    *) usage ;;
  esac
done

OPTIONS="-DCMAKE_INSTALL_PREFIX=/ -DCMAKE_BUILD_TYPE=${BUILD_TYPE}"

if [ ${VERBOSE} -eq 1 ]; then
  OPTIONS="${OPTIONS} -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON"
fi

mkdir -p build
pushd build
cmake ${OPTIONS} .. "$@"
make -j 20 install DESTDIR=../install
popd 
