#!/bin/bash

# copyright Antonio Barbalace, SSRG, VT, 2013

KERNEL=$1
BOOT_ARGS=$2
CPU=$3
BOOT_ADDR=$4
RAMDISK_ADDR=$5
RAMDISK_FILE="ramdisk.img"
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

echo "MKLINUX: try loading $RAMDISK_FILE at address $RAMDISK_ADDR"
RESULT=`./copy_ramdisk $RAMDISK_ADDR $RAMDISK_FILE 2>&1` 
if [ $? -ne 0 ]
then
  echo "ERROR TRACE: copy_ramdisk output:"
  echo "$RESULT"
  exit 1
fi

echo "MKLINUX: setting boot arguments to $BOOT_ARGS"
RESULT=`./wr_cmdline $BOOT_ARGS 2>&1`
if [ $? -ne 0 ]
then
  echo "ERROR TRACE: wr_cmdline output:"
  echo "$RESULT"
  ./rd_cmdline
  exit 1
fi
#./rd_cmdline

echo "MKLINUX: loading $KERNEL at address $BOOT_ADDR"
RESULT=`kexec -d -a $BOOT_ADDR -l $KERNEL -t elf-x86_64 --args-none 2>&1`
if [ $? -ne 0 ]
then
  echo "ERROR TRACE: kexec output (loading kernel into memory):"
  echo "$RESULT"
  exit 1
fi

echo "MKLINUX: booting kernel on CPU $CPU"
RESULT=`kexec -d -a $BOOT_ADDR -b $CPU 2>&1`
if [ $? -ne 0 ]
then
  echo "ERROR TRACE: kexec output (starting a secondary kernel):"
  echo "$RESULT"
  exit 1
fi

echo "MKLINUX: started $KERNEL at $BOOT_ADDR on $CPU ramdisk $RAMDISK_FILE at $RAMDISK_ADDR"
