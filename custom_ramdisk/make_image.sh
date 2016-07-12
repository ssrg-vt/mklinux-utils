#!/bin/bash
# Main script to create a cpio ramdisk that can be used for booting Popcorn secondary kernels.
# Takes no params and produces a cpio initramfs named custom-initramfs.cpio.gz in the current directory

# The main goal is to create an standalone initramfs containing executables copied from the current linux installation.
# To include a program from the current linux install, create a new package in packages. See packages/base/ and packages/nice/ for examples.
# The base backage sets up busybox and uses busybox as init, which in turn reads inittab.

# This script must set image_root, packages_dir, and mklinux_utils for its sub-scripts.

set -eu -o pipefail

trap 'fail "caught signal"' HUP KILL QUIT

readonly verbose="n"

if [ $# -ne 1 ]; then
    echo "ERROR: mklinux_utils_dir not given. " >&2
    echo "USAGE: make_image.sh mklinux_utils_dir. Produces a cpio initramfs named custom-initramfs.cpio.gz in the current directory." >&2
    exit 1
fi

# get absolute path to the dir containing the scripts
declare scripts_dir="$(dirname "$0")/"
pushd . > /dev/null
cd "$scripts_dir"
declare -r scripts_dir="$(pwd)/"
popd > /dev/null

# load utility function definitions
. "${scripts_dir}/utils.sh"

[ -d $1 ] || fail "directory $1 not found"
declare -r mklinux_utils="$(absolute_path "$1")"

declare image_root="$scripts_dir/image/"
declare packages_dir="$scripts_dir/packages/"

# check packages_dir and make sure we have its absolute path
is_dir packages_dir
declare -r packages_dir=$(absolute_path "$packages_dir")

# check image_root and make sure we have its absolute path
readonly root_parent=$(dirname "$image_root")
[ -d "$root_parent" ] || fail "directory $root_parent not found"
declare -r image_root="$(absolute_path "$root_parent")/$(basename "$image_root")/"

( . "$scripts_dir/create_image_fs.sh" ) || fail "$scripts_dir/create_image_fs.sh failed"
( . "$scripts_dir/package_image.sh" "$scripts_dir/custom-initramfs.cpio.gz") || fail "$scripts_dir/package_image.sh failed"
