#!/bin/sh

##########################################################################
# Copyright (c) 2013, 2014, SSRG VT.
# Author: Antonio Barbalace
#
# Copyright (c) 2007, 2008, 2009, ETH Zurich.
# All rights reserved.
#
# This file is distributed under the terms in the attached LICENSE file.
# If you do not find this file, copies can be found by writing to:
# ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
##########################################################################

MAXREP=24

MAXCORES=`cat /proc/cpuinfo | awk '/processor[ \t]+:[ \t]+[0-9]+/ {print $3}' | tail -n 1`
MAXCORES=`expr $MAXCORES + 1`
#echo $MAXCORES

rm -f results*.txt
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
    ./cg-gomp $i | grep $STRINGA >> results_cg_gomp.txt
    ./cg-bomp $i | grep $STRINGA >> results_cg_bomp.txt
    ./cg-pomp $i | grep $STRINGA >> results_cg_pomp.txt

    ./ft-gomp $i | grep $STRINGA >> results_ft_gomp.txt
    ./ft-bomp $i | grep $STRINGA >> results_ft_bomp.txt
    ./ft-pomp $i | grep $STRINGA >> results_ft_pomp.txt

    ./is-gomp $i | grep $STRINGA >> results_is_gomp.txt
    ./is-bomp $i | grep $STRINGA >> results_is_bomp.txt
    ./is-pomp $i | grep $STRINGA >> results_is_pomp.txt
  done
done

#    ./scalability-gomp $i >> results_scalability_gomp.txt
#    ./scalability-bomp $i >> results_scalability_bomp.txt

