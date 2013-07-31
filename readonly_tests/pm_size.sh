#!/bin/sh
RIPETI="1 2 3 4 5 6 7 8"
SIZES="0x10 0x100 0x1000 0x8000 0x10000 0x80000 0x100000 0x800000 0x1000000 0x8000000"
for SS in $SIZES
do
  for I in $RIPETI
  do 
    echo $SS $I
    pm_size $SS
    sleep 4 
  done
done
