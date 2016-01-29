#!/bin/bash
set -eu -o pipefail

mkdir -p $image_root/root/web/

cp -p ${scripts_dir}/packages/webroot/maemmaem.mp4 $image_root/root/web/
cp -p ${scripts_dir}/packages/webroot/index.html $image_root/root/web/
#( . "${scripts_dir}/simple_package.sh" nice )
