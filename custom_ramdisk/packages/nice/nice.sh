#!/bin/bash
set -eu -o pipefail

command -v nice >/dev/null 2>&1 || { echo >&2 "error: busybox is not installed.  Aborting."; exit 1; }
readonly nice="$(command -v nice)"

( . "${scripts_dir}"/copy_exec.sh "$nice" "$image_root")
