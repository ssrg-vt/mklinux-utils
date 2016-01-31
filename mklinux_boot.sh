#!/bin/bash

# copyright Antonio Barbalace, SSRG, VT, 2013

KERNEL=$1
BOOT_ARGS=$2
CPU=$3
BOOT_ADDR=$4
RAMDISK_ADDR=$5
RAMDISK_FILE="ramdisk.img"
LOGFILE="mklinux_boot.log"

if [ "$#" == "1" ] ; then
   SETUP_FILE="./boot_args_${1}.param"
   LOGFILE="mklinux_boot_${1}.log"

   if [ -f $SETUP_FILE ] ; then
      KERNEL=`cat   $SETUP_FILE     | awk '{ print $1 }'`
      BOOT_ARGS=`cat  $SETUP_FILE   | awk '{ print $2 }'`
      CPU=`cat $SETUP_FILE          | awk '{ print $3 }'`
      BOOT_ADDR=`cat $SETUP_FILE    | awk '{ print $4 }'`
      RAMDISK_ADDR=`cat $SETUP_FILE | awk '{ print $5 }'`

      PARAMS=`cat $SETUP_FILE`
      echo PARAMS=$PARAMS
      echo KERNEL=$KERNEL
      echo BOOT_ARGS=$BOOT_ARGS
      echo CPU=$CPU
      echo BOOT_ADDR=$BOOT_ADDR
      echo RAMDISK_ADDR=$RAMDISK_ADDR
      echo RAMDISK_FILE=$RAMDISK_FILE
   else
	echo " Setup file  $SETUP_FILE missing .. quitting \n"
	exit
   fi
fi

if [ ! -f ./$BOOT_ARGS ] ; then 
	echo " Boot Args file  $BOOT_ARGS missing .. quitting \n"
	exit
fi
 

#exit
 
#RAMDISK=$6


# check the user is root
if [[ "$USER" != "root" ]] ; then
	echo "ERROR: only root can boot secondary kernels."
	exit 1
fi
# check for the right cmd line arguments
if [ -z "$RAMDISK_FILE" -o ! -f "$RAMDISK_FILE" -o ! -s "$RAMDISK_FILE" ] ; then
  echo "ERROR: INVALID ARGUMENTS ramdisk file does not exist ($RAMDISK_FILE)"
  exit 1
fi 
if [ -z "$KERNEL" -o ! -f "$KERNEL" -o ! -s "$KERNEL" ] ; then
  echo "ERROR: INVALID ARGUMENTS kernel exec does not exist ($KERNEL)"
  exit 1
fi
if [ -z "$BOOT_ARGS" -o ! -f "$BOOT_ARGS" -o ! -s "$BOOT_ARGS" ] ; then
  echo "ERROR: INVALID ARGUMENTS cmdline args file does not exist ($BOOT_ARGS)"
  exit 1
fi
if [ -z "$CPU" ] ; then
  echo "ERROR: INVALID ARGUMENTS cpu id not set  ($CPU)"
  exit 1
fi
if [ -z "$BOOT_ADDR" ] ; then
  echo "ERROR: INVALID ARGUMENTS kernel phys addr not set  ($BOOT_ADDR)"
  exit 1
fi
if [ -z "$RAMDISK_ADDR" ] ; then
  echo "ERROR: INVALID ARGUMENTS ramdisk phys addr not set  ($RAMDISK_ADDR)"
  exit 1
fi
#check the kernel image and the ramdisk img
_VMLINUX=`file $KERNEL | awk '/ELF/ {print $0}'`
if [ -z "$_VMLINUX" ]
then
  echo "ERROR: $KERNEL is not in elf format"
  exit 1
fi
_RAMDISK=`file $RAMDISK_FILE | awk '/compressed data/ {print $0}'`
if [ -z "$_RAMDISK" ]
then
  echo "ERROR: $RAMDISK_FILE is not a compressed image"
  exit 1
fi

# start logging
date >> $LOGFILE

echo "MKLINUX: try loading $RAMDISK_FILE at address $RAMDISK_ADDR"
RESULT=`./copy_ramdisk $RAMDISK_ADDR $RAMDISK_FILE 2>&1` 
if [ $? -ne 0 ]
then
  echo "ERROR TRACE: copy_ramdisk output:"
  echo "$RESULT"
  echo "$RESULT" >> $LOGFILE
  exit 1
fi
echo "$RESULT" >> $LOGFILE

echo "MKLINUX: setting boot arguments to $BOOT_ARGS"
RESULT=`./wr_cmdline $BOOT_ARGS 2>&1`
if [ $? -ne 0 ]
then
  echo "ERROR TRACE: wr_cmdline output:"
  echo "$RESULT"
  echo "$RESULT" >> $LOGFILE
  _CMDLINE=`./rd_cmdline`
  echo "$_CMDLINE"
  echo "$_CMDLINE" >> $LOGFILE
  exit 1
fi
_CMDLINE=`./rd_cmdline`
#echo "$_CMDLINE"
echo "$BOOT_ARGS" >> $LOGFILE
cat $BOOT_ARGS >> $LOGFILE
echo "rd_cmdline" >> $LOGFILE
echo "$_CMDLINE" >> $LOGFILE

echo "MKLINUX: loading $KERNEL at address $BOOT_ADDR"

KEXEC=/home/giuliano/mklinux-kexec/build/sbin/kexec

RESULT=`$KEXEC -d -a $BOOT_ADDR -l $KERNEL -t elf-x86_64 --args-none 2>&1`
if [ $? -ne 0 ]
then
  echo "ERROR TRACE: kexec output (loading kernel into memory):"
  echo "$RESULT"
  echo "$RESULT" >> $LOGFILE
  exit 1
fi
echo "$RESULT" >> $LOGFILE

echo "MKLINUX: booting kernel on CPU $CPU"
RESULT=`$KEXEC -d -a $BOOT_ADDR -b $CPU 2>&1`
if [ $? -ne 0 ]
then
  echo "ERROR TRACE: kexec output (starting a secondary kernel):"
  echo "$RESULT"
  echo "$RESULT" >> $LOGFILE
  exit 1
fi
echo "$RESULT" >> $LOGFILE

echo "MKLINUX: started $KERNEL at $BOOT_ADDR on $CPU ramdisk $RAMDISK_FILE at $RAMDISK_ADDR"
