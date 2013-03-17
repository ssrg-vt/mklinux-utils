#!/bin/sh
echo MKLINUX: loading ramdisk...
./copy_ramdisk 0xa0000000
echo MKLINUX: setting boot arguments...
./set_boot_args bootargs_antonio_2.txt
echo MKLINUX: loading kernel...
kexec -d -a 0x80000000 -l vmlinux.elf -t elf-x86_64 --args-none
echo MKLINUX: booting kernel...
kexec -d -a 0x80000000 -b 2

