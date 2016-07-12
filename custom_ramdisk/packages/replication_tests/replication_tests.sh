#!/bin/bash
set -eu -o pipefail


# set up the server test in mklinux-utils/benchmarks/TCPEcho/
src_dir="${mklinux_utils}/benchmarks/TCPEcho/"
server_c="TCPEchoServerConnectionsDie.c"
command -v gcc >/dev/null 2>&1 || fail "gcc not found"
( cd ${src_dir}; gcc -static -o server "./${server_c}" ) || fail "failed to compile ${server_c}"

target_dir="${image_root}/${mklinux_utils}/ns/"
[ "${verbose}" = "y" ] && echo "target_dir is ${target_dir}"
mkdir -p "$target_dir"
( . "${scripts_dir}/copy_exec.sh" "${src_dir}/server" "$target_dir/" "server")

src_dir="${mklinux_utils}/benchmarks/racey-tcp/"
server_c="racey-tcp-server.c"
command -v gcc >/dev/null 2>&1 || fail "gcc not found"
( cd ${src_dir}; gcc -static -pthread -o racey-tcp-server "./${server_c}" ) || fail "failed to compile ${server_c}"i

( . "${scripts_dir}/copy_exec.sh" "${src_dir}/racey-tcp-server" "$target_dir/" "racey-tcp-server")

server_c="racey-tcp-webserver.c"
command -v gcc >/dev/null 2>&1 || fail "gcc not found"
( cd ${src_dir}; gcc -static -pthread -o racey-tcp-webserver "./${server_c}" ) || fail "failed to compile ${server_c}"i

( . "${scripts_dir}/copy_exec.sh" "${src_dir}/racey-tcp-webserver" "$target_dir/" "racey-tcp-webserver")

