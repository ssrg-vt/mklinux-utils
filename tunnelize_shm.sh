#!/bin/sh

VTY_ADDR=0x1ffc000000
#TUN_ADDR=0x1fec000000 # gigi
#TUN_ADDR=0xfbc000000 # found
TUN_ADDR=0x100000000 # QEMU
REPRESENTATIVE=`cat /proc/cpuinfo | grep processor | awk '{print $3}' | head -n 1`
TUN_CPU=$(( $REPRESENTATIVE + 1 ))
echo $TUN_ADDR

NICE="nice -n 20"
TUNNEL=""
[ -x "/home/${USER}/mklinux/mklinux-utils/tunnel_shm" ] && TUNNEL="/home/${USER}/mklinux/mklinux-utils/tunnel_shm"
[ -x "./tunnel_shm" ] && TUNNEL="./tunnel_shm"
which tunnel_shm && TUNNEL="tunnel_shm"

$NICE $TUNNEL $TUN_ADDR $REPRESENTATIVE &
sleep 2
echo "Done waiting for tunnel to open; setting up interface..."
TUN_ID=`ip -f inet link show |  awk '/shmtun[0-9]:/ {print $2}' | tail -n 1`
#TUN_DEV=${TUN_ID#tun}
TUN_DEV=${TUN_ID%:}
ifconfig $TUN_DEV 10.1.2.$TUN_CPU up
route add -net 10.1.2.0 netmask 255.255.255.0 dev $TUN_DEV
echo "tunnel device $TUN_DEV setup on 10.1.2.$TUN_CPU"

