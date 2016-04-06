#! /bin/sh
#
set -eu -o pipefail

( . "${scripts_dir}/simple_package.sh" /usr/local/bin/ffserver)
#mkdir -p "${image_root}/usr/local/bin/" 
#cp /usr/local/bin/ffserver "${image_root}/usr/local/bin/"
#( . "${scripts_dir}/copy_exec.sh" "/usr/local/bin/ffserver" "$image_root/usr/local/bin/" "ffserver")
cp "${mklinux_utils}/ns/ffserver.conf" "${image_root}/${mklinux_utils}/ns/" 
mkdir "${image_root}/tmp"
