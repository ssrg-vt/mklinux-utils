#!/bin/bash
set -eu -o pipefail

# assumes that image_root and packages_dir are set.

# clean the image root
if [ -d "$image_root" ]; then
    echo "cleaning $image_root"
    rm -r "$image_root"
    mkdir "$image_root"
else
    mkdir "$image_root"
fi

# create the basic fs structure
mkdir -p ${image_root}/{bin,dev,etc,lib,lib64,mnt/root,proc,root,sbin,sys}

# install packages
inst_packages() {
    local p dir script
    for p in $(ls -1 "${packages_dir}/"); do
        dir="${packages_dir}/$p/"
        if [ -d "$dir" ]; then
            script="$dir/$(basename "$dir").sh"
            ( cd "$dir"; . "$script" ) || fail "running $script failed"
        fi
    done
}
inst_packages
