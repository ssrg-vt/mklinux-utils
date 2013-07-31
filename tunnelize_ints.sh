#!/bin/sh

VTY_ADDR=0x200000000
#TUN_ADDR=0x1fec000000 # gigi
#TUN_ADDR=0xfbc000000 # found
TUN_ADDR=0x100000000 # QEMU
REPRESENTATIVE=`cat /proc/cpuinfo | grep processor | awk '{print $3}' | head -n 1`
TUN_CPU=$(( $REPRESENTATIVE + 1 ))
echo $TUN_ADDR $1

NICE="nice -n 20"
TUNNEL=""
[ -x "/home/bshelton/mklinux/mklinux-utils/tunnel_ints" ] && TUNNEL="/home/bshelton/mklinux/mklinux-utils/tunnel_ints"
[ -x "./tunnel_ints" ] && TUNNEL="./tunnel_ints"
which tunnel_ints && TUNNEL="tunnel_ints"

$NICE $TUNNEL $TUN_ADDR $REPRESENTATIVE &
sleep 2
echo "Done waiting for tunnel to open; setting up interface..."
TUN_ID=`ip -f inet link show |  awk '/tun[0-9]:/ {print $2}' | tail -n 1`
#TUN_DEV=${TUN_ID#tun}
TUN_DEV=${TUN_ID%:}
ifconfig $TUN_DEV 10.1.2.$TUN_CPU up
route add -net 10.1.2.0 netmask 255.255.255.0 dev $TUN_DEV
echo "tunnel device $TUN_DEV setup on 10.1.2.$TUN_CPU"

