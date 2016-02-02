#! /bin/sh
#
set -eu -o pipefail

#VER=3.6
#if [ ! -f mongoose-$VER.tgz ]; then
#    wget http://mongoose.googlecode.com/files/mongoose-$VER.tgz
#fi
#tar zxvf mongoose-$VER.tgz
#cd mongoose
#make linux
#mv mongoose ../mg-server
#cd ..
#( . "${scripts_dir}/copy_exec.sh" "mg-server" "$image_root")
cp mg-server $image_root/
cp mg-server /

