#!/bin/bash

KERNEL=$1
BOOT_ARGS=$2
CPU=$3
BOOT_ADDR=$4
RAMDISK_ADDR=$5
RAMDISK_FILE="ramdisk.img"
#RAMDISK=$6

if [[ "$USER" != "root" ]] ; then
	echo "Only root can boot secondary kernels."
	exit 1
fi

if [[ $RAMDISK_FILE $RAMDISK_ADDR ]]
then
  exit 1
fi 
echo "MKLINUX: try loading $RAMDISK_FILE at address $RAMDISK_ADDR"
$RESULT=`./copy_ramdisk $RAMDISK_ADDR $RAMDISK_FILE` 
if [ $? ]
then
  exit 1
fi

if [[ $BOOT_ARGS ]]
then
  exit 1
fi
echo "MKLINUX: setting boot arguments to $BOOT_ARGS"
./wr_cmdline $BOOT_ARGS
if [ $? ]
then
  exit 1
fi
./rd_cmdline
 
if [[ $KERNEL $BOOT_ADDR ]]
then
  exit 1
fi
echo "MKLINUX: loading $KERNEL at address $BOOT_ADDR"
kexec -d -a $BOOT_ADDR -l $KERNEL -t elf-x86_64 --args-none
if [ $? ]
then
#check for eventual error codes in the return
  exit 1
fi

exit 0

echo "MKLINUX: booting kernel ON CPU $CPU"
kexec -d -a $BOOT_ADDR -b $CPU
if [ $? ]
do
check for errors .. at this point is useless but, ok!
  exit 1
done
