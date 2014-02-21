# Copyright (c) 2013, 2014, VT.
#
# Copyright (c) 2007, 2008, 2009, ETH Zurich.
# All rights reserved.
#
# This file is distributed under the terms in the attached LICENSE file.
# If you do not find this file, copies can be found by writing to:
# ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.

DBGFLAGS=-g
OPTFLAGS=-O2

all: all-gomp all-bomp all-pomp

all-gomp: cg-gomp ft-gomp is-gomp ep-gomp
all-bomp: cg-bomp ft-bomp is-bomp ep-bomp
all-pomp: cg-pomp ft-pomp is-pomp ep-pomp
all-scalability: scalability-gomp scalability-bomp scalability-gomp

clean:
	rm -f cg-gomp cg-bomp ft-gomp ft-bomp is-gomp is-bomp


cg-gomp:
	gcc -o cg-gomp cg.c c_print_results.c c_randdp.c c_timers.c wtime.c -DPOSIX -lm -fopenmp $(OPTFLAGS) $(DBGFLAGS)

ep-gomp:
	gcc -o ep-gomp ep.c c_print_results.c c_randdp.c c_timers.c wtime.c -DPOSIX -lm -fopenmp $(OPTFLAGS) $(DBGFLAGS)

ft-gomp:
	gcc -o ft-gomp ft.c c_print_results.c c_randdp.c c_timers.c wtime.c -DPOSIX -lm -fopenmp $(OPTFLAGS) $(DBGFLAGS)

is-gomp:
	gcc -o is-gomp is.c c_print_results.c c_timers.c wtime.c -DPOSIX -lm -fopenmp $(OPTFLAGS) $(DBGFLAGS)

cg-bomp:
	gcc -c wtime.c -DPOSIX -g -O2
	gcc -o cg-bomp cg.c c_print_results.c c_randdp.c c_timers.c wtime.o -DBOMP -lm -fopenmp ../lib/libbomp.a -lpthread $(OPTFLAGS) $(DBGFLAGS)
	
ep-bomp:
	gcc -c wtime.c -DPOSIX -g -O2
	gcc -o ep-bomp ep.c c_print_results.c c_randdp.c c_timers.c wtime.o -DBOMP -lm -fopenmp ../lib/libbomp.a -lpthread $(OPTFLAGS) $(DBGFLAGS)

ft-bomp:
	gcc -c wtime.c -DPOSIX -g -O2
	gcc -o ft-bomp ft.c c_print_results.c c_randdp.c c_timers.c wtime.o -DBOMP -lm -fopenmp ../lib/libbomp.a -lpthread $(OPTFLAGS) $(DBGFLAGS)

is-bomp:
	gcc -o wtime.o -c wtime.c -DPOSIX -g -O2
	gcc -o is-bomp is.c c_print_results.c c_timers.c wtime.o -DBOMP -lm -fopenmp ../lib/libbomp.a -lpthread $(OPTFLAGS) $(DBGFLAGS)

cg-pomp:
	gcc -c wtime.c -DPOSIX -g -O2
	gcc -c cg.c c_print_results.c c_randdp.c c_timers.c -DBOMP -fopenmp $(OPTFLAGS) $(DBGFLAGS)
	gcc -o cg-pomp cg.o wtime.o c_print_results.o c_randdp.o c_timers.o ../lib/libpomp.a -lm $(DBGFLAGS)

ep-pomp:
	gcc -c wtime.c -DPOSIX -g -O2
	gcc -c ep.c c_print_results.c c_randdp.c c_timers.c wtime.o -DBOMP -fopenmp $(OPTFLAGS) $(DBGFLAGS)
	gcc -o ep-bomp ep.o wtime.o c_print_results.o c_timers.c ../lib/libpomp.a -lm $(DBGFLAGS)

ft-pomp
	gcc -c wtime.c -DPOSIX -g -O2
	gcc -c ft.c c_print_results.c c_randdp.c c_timers.c wtime.o -DBOMP -fopenmp $(OPTFLAGS) $(DBGFLAGS)
	gcc -o ft-bomp ft.o wtime.o c_print_results.o c_randdp.o c_timers.o ../lib/libbomp.a -lm $(DBGFLAGS)

is-bomp:
	gcc -o wtime.o -c wtime.c -DPOSIX -g -O2
	gcc -c is.c c_print_results.c c_timers.c wtime.o -DBOMP -fopenmp $(OPTFLAGS) $(DBGFLAGS)
	gcc -o is-bomp is.o wtime.o c_print_results.o c_timers.c ../lib/libbomp.a -lm $(DBGFLAGS)


scalability-gomp: clean
	gcc -o scalability-gomp scalability.c -DPOSIX -fopenmp $(OPTFLAGS) $(DBGFLAGS)

scalability-bomp: clean
	gcc -o scalability-bomp scalability.c -DBOMP -fopenmp ../lib/libbomp.a -lpthread -lnuma $(OPTFLAGS) $(DBGFLAGS)
	
scalability-pomp: clean
	gcc -c scalability.c -DBOMP -fopenmp $(OPTFLAGS) $(DBGFLAGS)
	gcc -o scalability-pomp scalability.o ../lib/libpomp.a $(DBGFLAGS)
	
