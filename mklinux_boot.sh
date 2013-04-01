#!/bin/bash

if [[ "$USER" != "root" ]] ; then
	echo "Only root can boot new kernels."
	exit 1
fi

KERNEL=$1
BOOT_ARGS=$2
CPU=$3
BOOT_ADDR=$4
RAMDISK_ADDR=$5

echo "MKLINUX: loading ramdisk AT $RAMDISK_ADDR"
./copy_ramdisk $RAMDISK_ADDR
echo "MKLINUX: setting boot arguments FROM $BOOT_ARGS"
./set_boot_args $BOOT_ARGS
echo "MKLINUX: loading kernel AT $BOOT_ADDR"
kexec -d -a $BOOT_ADDR -l $KERNEL -t elf-x86_64 --args-none
echo "MKLINUX: booting kernel ON CPU $CPU"
kexec -d -a $BOOT_ADDR -b $CPU
