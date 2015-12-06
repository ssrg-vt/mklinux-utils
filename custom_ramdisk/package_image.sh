#!/bin/bash
set -eu -o pipefail


[ $# -eq 1 ] || fail "ERROR: package_image.sh must be passed the name of the image to create"

. ./utils.sh

declare target=$1
[ -d "$(dirname "$target")" ] || fail "parent dir of $target found"

# get the absolute path of $target
declare -r target="$(absolute_path "$(dirname "$target")")/$(basename "$target")"

# do this in a sub-shell to avoid changing directory.
(cd "$image_root" && find . -print0 | cpio --null -ov --format=newc | gzip -9 > "$target") || fail "creation of the cpio archive failed"
