#!/bin/sh

#MAXREP=24
MAXREP=8

MAXCORES=`cat /proc/cpuinfo | awk '/processor[ \t]+:[ \t]+[0-9]+/ {print $3}' | tail -n 1`
MAXCORES=`expr $MAXCORES + 1`
#echo $MAXCORES

rm -f results_scalability_*.txt
#CORES="1 "
#CORES+=`seq 2 2 $MAXCORES`
CORES="1 "`seq 2 2 $MAXCORES`
REPEAT=`seq 1 $MAXREP`
STRINGA="Computetime"

for i in $CORES ;
do
  echo "running on $MAXREP exp on $i core/s.."
  for l in $REPEAT ;
  do
    ./scalability-gomp $i | grep $STRINGA >> results_scalability_gomp.txt
    ./scalability-bomp $i | grep $STRINGA >> results_scalability_bomp.txt
    ./scalability-pomp $i | grep $STRINGA >> results_scalability_pomp.txt
  done
done


