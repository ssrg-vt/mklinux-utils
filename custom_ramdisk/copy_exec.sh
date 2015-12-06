#!/bin/bash
# copies an executable along with all its dependencies to a destination directory.
# adapted from initramfs-tools

verbose="y"

# $1 = file to copy to ramdisk
# $2 = destination dir
# We never overwrite the target if it exists.

src="${1}"
DESTDIR="${2}"

[ -f "${src}" ] || exit 1

[ -e "${DESTDIR}/$src" ] && exit 0
mkdir -p "${DESTDIR}/${src%/*}"

[ "${verbose}" = "y" ] && echo "Adding binary ${src}"
cp -pL "${src}" "${DESTDIR}/${src}"

# Copy the dependant libraries
for x in $(ldd "${src}" 2>/dev/null | sed -e '
    /\//!d;
    /linux-gate/d;
    /=>/ {s/.*=>[[:blank:]]*\([^[:blank:]]*\).*/\1/};
    s/[[:blank:]]*\([^[:blank:]]*\) (.*)/\1/' 2>/dev/null); do

    libname=$(basename "${x}")
    dirname=$(dirname "${x}")

    #		mkdir -p "${DESTDIR}/${dirname}"
    if [ ! -e "${DESTDIR}/${dirname}/${libname}" ]; then
        # if the target dir does not exist, create it:
        if [ ! -d "${DESTDIR}/${dirname}" ]; then
            mkdir -p "${DESTDIR}/${dirname}"
        fi
        cp -pL "${x}" "${DESTDIR}/${dirname}/"
        [ "${verbose}" = "y" ] && echo "Added library ${x}" || true
    fi
done
