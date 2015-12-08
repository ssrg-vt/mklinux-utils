#!/bin/bash
set -eu -o pipefail

# This script installs busybox in $image_root

command -v busybox >/dev/null 2>&1 || { echo >&2 "error: busybox is not installed.  Aborting."; exit 1; }
readonly busybox=$(command -v busybox)

"${scripts_dir}/copy_exec.sh" "$busybox" "$image_root"

# create a symlink from /init to /bin/busybox
ln -s /bin/busybox "${image_root}/init"

# create a symlink for each busybox utility
( . ./create_busybox_links.sh ) || fail "running ./create_busybox_links.sh failed"

# copy inittab and rc.S
# TODO: copy shadow from the linux install instead
readonly files="etc/inittab etc/init.d/rc.S /bin/tunnelize.sh /bin/tunnel /bin/heartbeat etc/shadow etc/passwd etc/group"
copy_files() {
    local f src target dir
    for f in $files; do
        src="./"$(basename "$f")
        [ -f "$src" ] || fail "$(absolute_file_path "$src") not found"
        target="${image_root}/$f"
        dir=$(dirname "$target")
        mkdir -p "$dir"
        cp "$src" "$target"
    done
}
copy_files

# set up udhcpc
readonly udhcpc_default_script="/etc/udhcpc/default.script"
if [ ! -f "$udhcpc_default_script" ]; then
    echo >&2 "error: $udhcpc_default_script not found. Aborting"
    exit 1
else
    mkdir "${image_root}/etc/udhcpc/"
    cp "$udhcpc_default_script"  "${image_root}/etc/udhcpc/"
fi
