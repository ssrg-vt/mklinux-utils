#! /bin/sh
#
set -eu -o pipefail

cd mongoose
make linux
mv mongoose ../mg-server
cd ..
( . "${scripts_dir}/copy_exec.sh" "mg-server" "$image_root")
