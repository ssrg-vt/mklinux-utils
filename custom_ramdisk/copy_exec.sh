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

[ "${verbose}" = "y" ] && echo "copy_exec.sh: looking at ${src}"
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

if [ ! -e "${target}" ]; then
    [ "${verbose}" = "y" ] && echo "Copying binary ${src} to ${target}"
    cp -pL "${src}" "${target}"
fi

# Copy the needed libraries
for x in $(ldd "${src}" 2>/dev/null | sed -e '
    /\//!d;
    /linux-gate/d;
    /=>/ {s/.*=>[[:blank:]]*\([^[:blank:]]*\).*/\1/};
    s/[[:blank:]]*\([^[:blank:]]*\) (.*)/\1/' 2>/dev/null); do

    # x is an absolute path
    libname=$(basename "${x}")
    dirname=$(dirname "${x}")
    [ "${verbose}" = "y" ] && echo "Adding library ${x}" 

    if [ ! -e "${dest}/$x" ]; then
        [ "${verbose}" = "y" ] && echo "Not yet copied: $x" 
        # if the target dir does not exist, create it:
        if [ ! -d "${dest}/${dirname}" ]; then
            [ "${verbose}" = "y" ] && echo "Creating ${dest}/${dirname}" 
            mkdir -p "${dest}/${dirname}"
        fi
        if [ -h "$x" ]; then
            [ "${verbose}" = "y" ] && echo "Copying symlink $x" 
            # get target
            link_target="$(readlink -f "$x")"
            # create the link.
            ln -s "$link_target" "${dest}/$x"
            # copy the target if needed
            if [ ! -e "${dest}/$link_target" ]; then
                [ "${verbose}" = "y" ] && echo "Copying target $link_target of symlink $x" 
                cp -p "$link_target" "${dest}/$link_target"
            fi
        else
            cp -pL "${x}" "${dest}/${dirname}/"
        fi
    fi
done

# TODO: make sure that the lib dirs are in LD_LIBRARY_PATH
