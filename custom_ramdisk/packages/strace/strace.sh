#!/bin/bash
set -eu -o pipefail

command -v strace >/dev/null 2>&1 || { echo >&2 "error: strace is not installed.  Aborting."; exit 1; }
readonly strace=$(command -v strace)

( . "${scripts_dir}/copy_exec.sh" "$strace" "$image_root")

