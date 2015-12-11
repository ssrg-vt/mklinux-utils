#!/bin/bash
set -eu -o pipefail

readonly cmd="$1"
command -v "$cmd" >/dev/null 2>&1 || { echo >&2 "error: $cmd is not installed.  Aborting."; exit 1; }
readonly cmd_path=$(command -v "$cmd")

( . "${scripts_dir}/copy_exec.sh" "$cmd_path" "$image_root")

