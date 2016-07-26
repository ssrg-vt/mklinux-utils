#!/bin/bash
set -eu -o pipefail

INSTALL_LOCATION=/usr/local/npb

CPUS=`cat /proc/cpuinfo | grep processor | wc -l`

make CFLAGS+=-I../common C_LIB+=-lm suite -j$CPUS

mkdir -p $INSTALL_LOCATION

cp bin/* $INSTALL_LOCATION

for prog in `ls $INSTALL_LOCATION`; do
	( . "${scripts_dir}/simple_package.sh" $INSTALL_LOCATION/$prog)
done

# ( . "${scripts_dir}/simple_package.sh" /usr/local/nginx/sbin/nginx)
# cp -r $INSTALL_LOCATION "${image_root}$INSTALL_LOCATION"
