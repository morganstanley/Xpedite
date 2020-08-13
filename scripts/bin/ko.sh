#!/usr/bin/env bash
################################################################################################
#
# Xpedite kernel module  management
#
# Author: Manikandan Dhamodharan, Morgan Stanley
#
################################################################################################

function unload()
{
  if $(cat /proc/modules | grep '^xpedite[ ]' > /dev/null) ; then
    echo unloading Xpedite kernel module
    /sbin/rmmod xpedite.ko
  else
    echo Xpedite kernel module not loaded 1>&2
    exit 2
  fi
}

function load()
{
  SCRIPT=$(/usr/bin/readlink -f $0)
  INSTALL_DIR="$(dirname ${SCRIPT})/../.."
  INSTALL_DIR=$(/usr/bin/readlink -f $INSTALL_DIR)
  RELEASE=$(basename $INSTALL_DIR)
  KERNEL_RELEASE=$(uname -r)
  KERNEL_MODULE=${INSTALL_DIR}/modules/${KERNEL_RELEASE}/xpedite.ko

  # fall back to lsb_release string
  if [ ! -e ${KERNEL_MODULE} ] ; then
          echo "Xpedite release ${RELEASE} does not support kernel version ${KERNEL_RELEASE}" 2>&1 ; exit 1
  fi
  
  if [ -e /proc/sys/kernel/nmi_watchdog ]; then
    WATCH_DOG_ALIVE=$(cat /proc/sys/kernel/nmi_watchdog)
    if [ $WATCH_DOG_ALIVE != "0" ]; then
      cat << EOM
        usage of watchdog NMI interrupts can interfere with Xpedite use of performance coutners.
        Xpedite will disabling NMI Watch dog now ...
EOM
      echo 0 > /proc/sys/kernel/nmi_watchdog
    fi
  fi

  if [ -e '/sys/module/xpedite' ]; then
    if [ ${FORCE} -eq 1 ] ; then
      echo Xpedite moule already loaded, force reloading ...
      /sbin/rmmod xpedite.ko
    else
      echo Xpedite kernel module already loaded
      exit 2
    fi
  fi
  
  if /sbin/insmod ${KERNEL_MODULE}; then
    echo Xpedite kernel module loaded successfully
    if [ -e /dev/xpedite ]; then
      chmod 666 /dev/xpedite
    fi
  else
    echo Failed to load Xpedite kernel module 2>&1
    exit 2
  fi
}

function usage() {
cat << EOM
Usage: $0 [OPTION]...
Load/UnLoad Xpedite kernel module.

  -v, --verbose              verbose mode
  -l, --load                 load xpedite kernel module
  -u, --unload               unload xpedite kernel module
  -f, --force                force load
EOM
exit 1
}

ARGS=$(getopt -o vluf --long verbose,load,unload,force -- "$@")
if [ $? -ne 0 -o $# -le 0 ]; then
  usage
fi

eval set -- "$ARGS"

FORCE=0
VERBOSE=0
ACTION=''
while true ; do
  case "$1" in
    -l|--load)
        ACTION='load' ; shift ;;
    -u|--unload)
        ACTION='unload' ; shift ;;
    -f|--force)
        FORCE=1; shift ;;
    -v|--verbose)
        echo 'verbose'
        VERBOSE=1 ; shift ;;
    --) shift ; break ;;
    *) usage ;;
  esac
done

if [ $VERBOSE -eq 1 ]; then
  set -x 
fi

case ${ACTION} in
  'load')
    load ;;
  'unload')
    unload ;;
esac
