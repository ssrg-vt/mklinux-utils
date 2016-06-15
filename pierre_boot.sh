#!/bin/sh

echo "" > mklinux_boot.log
./tunnelize.sh
./mklinux_boot.sh `cat boot_args_1.param`
