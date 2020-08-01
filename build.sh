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
  -c, --withCallStacks            build support for tracing call stacks
  -j, --forJava                  build support for profiling java apps
  -v, --verbose                   collect hardware performance counters
EOM
exit 1
}

ARGS=`getopt -o t:cjv --long type:,withCallStacks,forJava,verbose -- "$@"`
if [ $? -ne 0 ]; then
  usage
fi

eval set -- "$ARGS"
BUILD_TYPE=Release
BUILD_VIVIFY=0
BUILD_JAVA=0
VERBOSE=0

while true ; do
  case "$1" in
    -t|--type)
        BUILD_TYPE=$2 ; shift 2 ;;
    -c|--withCallStacks)
        BUILD_VIVIFY=1 ; shift ;;
    -j|--forJava)
        BUILD_JAVA=1 ; shift ;;
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

if [ ${BUILD_VIVIFY} -eq 1 ]; then
  if pkg-config --help >/dev/null 2>&1; then
    OPTIONS="${OPTIONS} -DBUILD_VIVIFY=ON"
  else
    echo xpedite callstack support requires pkg-config util. Please install pkg-config and run this script again
    exit 1
  fi
fi

if [ ${BUILD_JAVA} -eq 1 ]; then
  OPTIONS="${OPTIONS} -DBUILD_JAVA=ON"
fi

mkdir -p build
pushd build
cmake ${OPTIONS} .. "$@"
make -j 20 install DESTDIR=../install
RC=$?
popd 
exit $RC
