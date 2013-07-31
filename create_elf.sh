#!/bin/bash

# Antonio Barbalace, SSRG, VT, 2013

# script copied from the Linux kernel source
# have a look at arch/x86/boot/compressed/Makefile
# argument is a vmlinux image (kernel's root source directory)

# check the argument si not null
if [ -z $1 ]
then
  echo "error. specifiy vmlinux image (uncompressed bzImage)"
  exit 1
fi
# check the existence of the file 
if [ ! -e $1 ]
then
  echo "error. file path not correct"
  exit 1
fi
# check the file is not null
if [ ! -s $1 ] 
then
  echo "error. the file is empty"
  exit 1
fi
# check the file is in ELF format
_VMLINUX=`file $1 | awk '/ELF/ {print $0}'`
if [ -z "$_VMLINUX" ]
then
  echo "error. $1 is not in elf format"
  exit 1
fi
# do the actual objcopy
objcopy -R .notes -R .comment -S $1 vmlinux.elf 
if [ $? -ne 0 ]
then 
  echo "error creating vmlinux.elf .."
  exit 1
fi
exit 0
