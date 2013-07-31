#!/bin/bash

if [[ "$USER" != "root" ]] ; then
	echo "Only root can boot secondary kernels."
	exit 1
fi

KERNEL=$1
BOOT_ARGS=$2
CPU=$3
BOOT_ADDR=$4
RAMDISK_ADDR=$5

echo "MKLINUX: loading ramdisk AT $RAMDISK_ADDR"
./copy_ramdisk $RAMDISK_ADDR
if [ $? ]
do
check for errors
  exit 1
done

echo "MKLINUX: setting boot arguments FROM $BOOT_ARGS"
./wr_cmdline $BOOT_ARGS
if [ $? ]
do
check for errors
  exit 1
done






echo "MKLINUX: loading kernel AT $BOOT_ADDR"
kexec -d -a $BOOT_ADDR -l $KERNEL -t elf-x86_64 --args-none
if [ $? ]
do
doe skexec returns errors?!
check for errors
  exit 1
done

echo "MKLINUX: booting kernel ON CPU $CPU"
kexec -d -a $BOOT_ADDR -b $CPU
if [ $? ]
do
check for errors .. at this point is useless but, ok!
  exit 1
done
