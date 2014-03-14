#!/bin/sh

#MAXREP=24
MAXREP=8
INPUT_FILE=graph1MW_6.txt

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
    rm result.txt
    ./bfs-gomp $i $INPUT_FILE | grep $STRINGA >> results_gomp.txt
    rm result.txt
    ./bfs-bomp $i $INPUT_FILE | grep $STRINGA >> results_bomp.txt
    rm result.txt
    ./bfs-pomp $i $INPUT_FILE | grep $STRINGA >> results_pomp.txt
  done
done


