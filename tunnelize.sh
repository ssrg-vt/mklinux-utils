#!/bin/sh

VTY_ADDR=0xb0000000
TUN_ADDR=0xb8000000
#TUN_ADDR=0x7a0000000 # phil
#TUN_ADDR=0xfbc000000 # found

TUNNEL_ADDR=`dmesg | sed -n '/virtualTTY:[ ]*cpu [0-9]*,[ ]*phys /p' | sed 's/ phys 0x[0-9a-f]*-\(0x[0-9a-f]*\)/ \1/' | awk '{print $NF}'`
if [ ! -z $TUNNEL_ADDR ]
then
  echo "fixing TUN_ADDR from $TUN_ADDR to $TUNNEL_ADDR"
  TUN_ADDR=$TUNNEL_ADDR
fi

REPRESENTATIVE=`cat /proc/cpuinfo | grep processor | awk '{print $3}' | head -n 1`
TUN_CPU=$(( $REPRESENTATIVE + 1 ))
echo tun_addr = $TUN_ADDR tun_cpu = $TUN_CPU 

#NICE="nice -n 20"
TUNNEL=""
[ -x "/home/${USER}/mklinux/mklinux-utils/tunnel" ] && TUNNEL="/home/${USER}/mklinux/mklinux-utils/tunnel"
[ -x "./tunnel" ] && TUNNEL="./tunnel"
which tunnel && TUNNEL="tunnel"

$TUNNEL $TUN_ADDR $REPRESENTATIVE &> /dev/null &
TUN_ID=""
while [ -z $TUN_ID ]
do
  TUN_ID=`ip -f inet link show |  awk '/tun[0-9]:/ {print $2}' | tail -n 1`
done

#TUN_DEV=${TUN_ID#tun}i
TUN_DEV=${TUN_ID%:}
echo "ifconfig $TUN_DEV 10.1.2.$TUN_CPU up"
ifconfig $TUN_DEV 10.1.2.$TUN_CPU up
route add -net 10.1.2.0 netmask 255.255.255.0 dev $TUN_DEV
echo "tunnel device $TUN_DEV setup on 10.1.2.$TUN_CPU"
