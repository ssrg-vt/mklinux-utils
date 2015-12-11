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
mkdir -p ${image_root}/{bin,dev,etc,lib,lib64,mnt/root,proc,root,sbin,sys,dev/pts}

# install packages in lexicographic order
# the convention is that a package dir must contain a script which has the same name as the directory, plus the .sh postfix, unless the dir name is of the form DDname, where D is a digit, in which case the script must be called name.sh.
# the DDname form can be used to enforce an order on packages.
declare package_dir install_script
for package_dir in $(ls -1l "${packages_dir}/" | grep '^d' | awk '{print $9}' | sort -u); do
    package_dir_abs="${packages_dir}/$package_dir/"
    install_script="$package_dir_abs/${package_dir##[[:digit:]][[:digit:]]}.sh"
    ( cd "$package_dir_abs"; . "$install_script" ) || fail "running $install_script failed"
done
