#!/bin/sh

VTY_ADDR=0x1ffc000000
#TUN_ADDR=0x1fec000000 # gigi
TUN_ADDR=0xfbc000000 # found
REPRESENTATIVE=`cat /proc/cpuinfo | grep processor | awk '{print $3}' | head -n 1`
TUN_CPU=$(( $REPRESENTATIVE + 1 ))
echo $TUN_ADDR $1

NICE="nice -n 20"
TUNNEL=""
[ -x "/home/bshelton/mklinux/mklinux-utils/tunnel" ] && TUNNEL="/home/bshelton/mklinux/mklinux-utils/tunnel"
[ -x "./tunnel" ] && TUNNEL="./tunnel"
which tunnel && TUNNEL="tunnel"

$NICE $TUNNEL $TUN_ADDR $REPRESENTATIVE &> /dev/null &
TUN_ID=`ip -f inet link show |  awk '/tun[0-9]:/ {print $2}' | tail -n 1`
#TUN_DEV=${TUN_ID#tun}
TUN_DEV=${TUN_ID%:}
ifconfig $TUN_DEV 10.1.2.$TUN_CPU up
route add -net 10.1.2.0 netmask 255.255.255.0 dev $TUN_DEV
echo "tunnel device $TUN_DEV setup on 10.1.2.$TUN_CPU"

