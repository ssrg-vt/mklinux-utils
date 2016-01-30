#!/bin/busybox sh
 
# Mount the /proc and /sys filesystems.
mount -t proc none /proc
mount -t sysfs none /sys
# Populate /dev
mount -t devtmpfs none /dev
mkdir /dev/pts
mount devpts /dev/pts -t devpts
# sleep in case mounting /dev takes time (is something synchronous?)
sleep 1

echo "starting tunnelize" 1>&2
/usr/bin/nice -n 20 /bin/tunnelize.sh
echo "after trying to start tunnelize, '$?' is $?" 1>&2
sleep 1
/bin/chmod 777 /dev/vty*
/bin/chown root /dev/vty*
echo "starting heartbeat" 1>&2
/bin/heartbeat 1
# MYIP is to be replaced by the ip address of the primary.
/sbin/ifconfig ft_dummy_driver MYIP
echo "end of rc.S" 1>&2

# configure eth0
#ifconfig eth0 up
#udhcpc -i eth0 -s "/etc/udhcpc/default.script"
