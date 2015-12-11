#!/bin/bash
# creates symlinks for all busybox utilities.
set -eu -o pipefail

# expects $image_root and $busybox to be set

readonly busybox_target="${image_root}$busybox"

if [ ! -f "$busybox_target" ]; then
    echo "error: busybox has not been copied to the image. Aborting."
    exit 1
fi

declare target
declare dirname
# set the field separator to newline, for the for loop below.
IFS=$(echo -en "\n\b")
for f in $(busybox --list-all); do
    target="${image_root}/$f"
    dirname=$(dirname "$target")
    if [ ! -d "$dirname" ]; then
        [ "${verbose}" = "y" ] && echo "creating directory $dirname"
        mkdir -p "$dirname"
    fi
    [ "${verbose}" = "y" ] && echo "creating symlink $target"
    ln -s "$busybox" "$target"
done
unset IFS
