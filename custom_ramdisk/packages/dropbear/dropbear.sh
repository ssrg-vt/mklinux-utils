#!/bin/bash
set -eu -o pipefail

command -v dropbear >/dev/null 2>&1 || { echo >&2 "error: dropbear is not installed.  Aborting."; exit 1; }
readonly dropbear=$(command -v dropbear)

( . "${scripts_dir}/copy_exec.sh" "$dropbear" "$image_root")

command -v dropbearkey >/dev/null 2>&1 || { echo >&2 "error: dropbearkey is not installed.  Aborting."; exit 1; }
readonly dropbearkey=$(command -v dropbearkey)


readonly dropbear_config_dir="${image_root}/etc/dropbear"
mkdir $dropbear_config_dir

cp "./dropbear_rsa_host_key" "${dropbear_config_dir}/"

#$dropbearkey -t rsa -f "$dropbear_config_dir/rsa_key"
#( . "${scripts_dir}/copy_exec.sh" "$dropbearkey" "$image_root")
