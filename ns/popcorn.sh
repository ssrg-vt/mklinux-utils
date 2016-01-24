#!/bin/sh

if [ -z "$1" ]
then
	echo "specify replication degree"

elif [ $1 -lt 2 ] || [ $1 -gt 64 ]
then
	echo "replication degree $1 is invalid"
else
	echo "switching to poopcorn ns! Replication degree requested is $1"
	echo $1 > /proc/popcorn_namespace
	exec /bin/busybox sh
fi

