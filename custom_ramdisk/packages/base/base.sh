#!/bin/bash
set -eu -o pipefail

# This script installs busybox in $image_root

command -v busybox >/dev/null 2>&1 || { echo >&2 "error: busybox is not installed.  Aborting."; exit 1; }
readonly busybox=$(command -v busybox)
# create copy and create a symlink from /init to /bin/busybox
( . "${scripts_dir}/copy_exec.sh" "$busybox" "$image_root")
# setuid root for busybox (for login features)
readonly new_busybox="$image_root/$busybox"
chown root:root "$new_busybox"
chmod u+s "$new_busybox"
# link /init to busybox
ln -s /bin/busybox "${image_root}/init"

# create a symlink for each busybox utility
( . ./create_busybox_links.sh ) || fail "running ./create_busybox_links.sh failed"

# now copy the tunnel executable from mklinux-utils
readonly tunnel="$mklinux_utils/tunnel"
( . "${scripts_dir}/copy_exec.sh" "$tunnel" "$image_root" "/bin/tunnel" )

# set up udhcpc
readonly udhcpc_default_script="/etc/udhcpc/default.script"
if [ ! -f "$udhcpc_default_script" ]; then
    echo >&2 "error: $udhcpc_default_script not found. Aborting"
    exit 1
else
    mkdir "${image_root}/etc/udhcpc/"
    cp -a "$udhcpc_default_script"  "${image_root}/etc/udhcpc/"
fi

# all the dirs containing dynamic libraries
readonly ldconfig_dirs="$(/sbin/ldconfig -p | grep '=>' | cut -d '>' -f 2 | rev | cut -d / -f 2- | rev | sort -u )"

# copy the libnss* libs, needed by login utilities
# set the field separator to newline, for the for loop below.
#readonly ldconfig_dirs="$(/sbin/ldconfig -p | grep '=>' | cut -d '>' -f 2 | rev | cut -d / -f 2- | rev | sort -u |  tr -d '\n')"
readonly ldconfig_dirs_one_line="$(echo "${ldconfig_dirs//[[:blank:]]/}" | tr '\n' ' ')"
for lib in $(find $ldconfig_dirs_one_line \( -type f -o -type l \) -name "libnss*"); do
        # TODO: do not create copies in place of symlinks...
        ( . "${scripts_dir}/copy_exec.sh" "$lib" "$image_root")
done

# populate /etc/profile
readonly profile="$(cat << EOF
PATH="$PATH"
export PATH
LD_LIBRARY_PATH="$(echo "${ldconfig_dirs//[[:blank:]]/}" | paste -d: -s )"
export LD_LIBRARY_PATH
EOF
)"
echo "$profile" > "./profile"

# setup /etc/ld.so.conf
echo "${ldconfig_dirs//[[:blank:]]/}" >> "${image_root}/etc/ld.so.conf"

copy_files() {
    # copy the following list of file from the current dir to ${image_root}/path_to_file
    local readonly files="etc/inittab etc/init.d/rc.S /bin/tunnelize.sh /bin/heartbeat etc/shadow etc/passwd etc/profile etc/nsswitch.conf"
    local f src target dir
    for f in $files; do
        src="./"$(basename "$f")
        [ -f "$src" ] || fail "$(absolute_file_path "$src") not found"
        target="${image_root}/$f"
        dir=$(dirname "$target")
        mkdir -p "$dir"
        cp -a "$src" "$target"
    done

}
copy_files
