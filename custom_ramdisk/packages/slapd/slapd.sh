#!/bin/bash
set -eu -o pipefail

version="2.4.43"
url="ftp://ftp.openldap.org/pub/OpenLDAP/openldap-release/openldap-${version}.tgz"
file="./openldap-${version}.tgz"
dir="./openldap-${version}"
# fetch
if [ ! -f "$file" ]; then
    wget "$url"
fi
# unpack
tar -xzf "$file"
# compile
( cd "$dir"; ./configure "--enable-debug"; make; make depend )
# copy to image
server="$dir/servers/slapd/slapd"
( . "${scripts_dir}/copy_exec.sh" "$server" "$image_root" "/root/slapd")
config="$dir/servers/slapd/slapd.conf"
cp "$config" "${image_root}/root/slapd.conf"

# remove the dir where we unpacked?
# rm -r "$dir"
