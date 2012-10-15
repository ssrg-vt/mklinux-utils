#!/bin/sh
VTY_ADDR=0x1ffc000000
TUN_ADDR=0x1fec000000
TUN_DEV="tun0"
TUN_CPU=$(( $1 + 1 ))
echo $TUN_ADDR $1
./tunnel $TUN_ADDR $1 &> /dev/null &
ifconfig $TUN_DEV 10.1.2.$TUN_CPU up
route add -net 10.1.2.0 netmask 255.255.255.0 dev $TUN_DEV
echo "tunnel device $TUN_DEV setup on 10.1.2.$TUN_CPU"

