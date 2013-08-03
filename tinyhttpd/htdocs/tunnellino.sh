#!/bin/sh
#/home/antoniob/mklinux/mklinux-utils/tunnel_html 0x1fec000000
ADDR=`ps x | awk '/ tunnel / {print $6}' | head -n 1`
[ -f ../tunnel_html ] && ../tunnel_html $ADDR ; exit 0
[ -f ../../tunnel_html ] && ../../tunnel_html $ADDR ; exit 0
echo "error tunnel_html not found"
exit 1
