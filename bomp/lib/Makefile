##########################################################################
# Copyright (c) 2007, 2008, 2009, ETH Zurich.
# All rights reserved.
#
# This file is distributed under the terms in the attached LICENSE file.
# If you do not find this file, copies can be found by writing to:
# ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
##########################################################################

CFLAGS = -DPOSIX -g -m64

# TODO --- add a switch to select linux_ or popcorn_
#libbomp.a: linux_backend.o omp.o parallel.o processing.o
#	$(AR) -q $@ $^

# TODO libbomp and libpomp
#libpomp.a: cthread.o popcorn_backend.o omp.o parallel.o processing.o
libbomp.a: cthread.o popcorn_backend.o omp.o parallel.o processing.o
	$(AR) -q $@ $^

clean:
	rm -f *.o libbomp.a libpomp.a
