#! /bin/sh
#
set -eu -o pipefail

make clean
make
cp pcn_det.so $image_root/usr/lib/
