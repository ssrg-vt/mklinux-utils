#!/bin/bash
set -eu -o pipefail

REPO_URL=https://github.com/nginx/nginx.git
TAG=release-1.10.1
CUSTOM_CONF="./nginx.conf"

CPUS=`cat /proc/cpuinfo | grep processor | wc -l`

if [ ! -e nginx ]; then
	git clone $REPO_URL
	cd nginx
	git checkout release-1.10.1
	./auto/configure --with-threads --without-http_rewrite_module --without-http_gzip_module --prefix=/usr/local/nginx
	make -j$CPUS
	sudo make install
	cd -
	cp -f $CUSTOM_CONF /usr/local/nginx/conf/nginx.conf
fi

( . "${scripts_dir}/simple_package.sh" /usr/local/nginx/sbin/nginx)
cp -r /usr/local/nginx "${image_root}/usr/local/"
