#!/bin/bash
set -eu -o pipefail

( . "${scripts_dir}/simple_package.sh" /usr/local/nginx/sbin/nginx)
cp -r /usr/local/nginx "${image_root}/usr/local/"
