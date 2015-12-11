#!/bin/bash
set -eu -o pipefail

target_dir="${image_root}/${mklinux_utils}/ns/"
[ "${verbose}" = "y" ] && echo "target_dir is ${target_dir}"
mkdir -p "$target_dir"
( . "${scripts_dir}/copy_exec.sh" "${scripts_dir}/../benchmarks/TCPEcho/server" "$target_dir/" "server")
