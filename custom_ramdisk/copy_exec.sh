#!/bin/bash
# copies the executable $1 along with all its dependencies to ${2}/$(absolute_file_path $1) or ${2}/$3 if $3 is given.
# adapted from initramfs-tools

# $1: file to copy to ramdisk
# $2: destination root dir
# $3: new location for the main exec (optional) 
# We never overwrite the target if it exists.

set -eu -o pipefail

declare src="${1}"
[ -f "${src}" ] || fail "file ${src} not found"
declare -r src="$(absolute_file_path "$src")"

[ "${verbose}" = "y" ] && echo "src is ${src}"

[ -d "$2" ] || fail "directory $2 not found"
declare -r dest="$2"

if [ $# -eq 3 ]; then
    declare -r target="$(absolute_path "$dest")/$3"
else
    declare -r target="$(absolute_path "$dest")/$src"
fi

[ "${verbose}" = "y" ] && echo "target is ${target}"

# [ ! -e "$target" ] || fail "$target already exists"

# create the dir to copy to if it doesn't exist
mkdir -p "${target%/*}"

[ "${verbose}" = "y" ] && echo "Copying binary ${src} to ${target}"
cp -pL "${src}" "${target}"

# Copy the dependant libraries
for x in $(ldd "${src}" 2>/dev/null | sed -e '
    /\//!d;
    /linux-gate/d;
    /=>/ {s/.*=>[[:blank:]]*\([^[:blank:]]*\).*/\1/};
    s/[[:blank:]]*\([^[:blank:]]*\) (.*)/\1/' 2>/dev/null); do

    libname=$(basename "${x}")
    dirname=$(dirname "${x}")

    #		mkdir -p "${dest}/${dirname}"
    if [ ! -e "${dest}/${dirname}/${libname}" ]; then
        # if the target dir does not exist, create it:
        if [ ! -d "${dest}/${dirname}" ]; then
            mkdir -p "${dest}/${dirname}"
        fi
        cp -pL "${x}" "${dest}/${dirname}/"
        [ "${verbose}" = "y" ] && echo "Added library ${x}" || true
    fi
done

# TODO: make sure that the lib dirs are in LD_LIBRARY_PATH
