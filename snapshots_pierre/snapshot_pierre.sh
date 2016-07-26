#!/bin/bash
set -euo pipefail

TMPDIR=/tmp/pierre_snapshot

# clean and / or create tmpdir
rm -rf $TMPDIR && mkdir -p $TMPDIR

# tunnelize scripts
cp ../tunnelize.sh $TMPDIR
cp ../custom_ramdisk/packages/01base/tunnelize.sh $TMPDIR/tunnelize.sh.secondary

# boot args and params
cp ../boot_args_1.param $TMPDIR
cp ../boot_args_1.args $TMPDIR

host=`hostname`
ker=`uname -r`
name="${host}.${ker}"

tar -C `dirname $TMPDIR` -czf ./$name.tar.gz `basename $TMPDIR`

echo "Created $name.tar.gz"
