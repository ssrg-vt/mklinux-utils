#!/bin/bash
set -eu -o pipefail

# downloads, compiles slapd, and copies it to the image.

version="2.4.43"
url="ftp://ftp.openldap.org/pub/OpenLDAP/openldap-release/openldap-${version}.tgz"
file="./openldap-${version}.tgz"
dir=$(absolute_path "./openldap-${version}")
# fetch
if [ ! -f "$file" ]; then
    wget "$url"
fi
# unpack
#tar -xzf "$file"
# compile
server="$dir/servers/slapd/slapd"
if [ ! -f "$server" ]; then
#    The following line is for automatic instrumentation
#    ( cd "$dir"; ./configure CC=clang CFLAGS="-O2 -Xclang -load -Xclang /home/wen/LLVMBalthasar.so"; make -j32; make depend )
    ( cd "$dir"; ./configure; make -j32; make depend )
fi
# copy to image
( . "${scripts_dir}/copy_exec.sh" "$server" "$image_root")
config="$dir/servers/slapd/slapd.conf"
cp "$config" "${image_root}/"$(dirname "$server")"/slapd.conf"
# copy other server files
servers="$dir/servers"
cp -r "$servers" "${image_root}/"$(dirname "$servers")
# copy the tests
tests="$dir/tests"
cp -r "$tests" "${image_root}/"$(dirname "$tests")
# copy the clients
clients="$dir/clients"
cp -r "$clients" "${image_root}/"$(dirname "$clients")

# remove the dir where we unpacked?
# rm -r "$dir"
