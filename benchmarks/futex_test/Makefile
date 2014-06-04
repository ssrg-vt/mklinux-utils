# Filename: Makefile
# # Description: The make file is to build up the crawler.
# # Warning:  See how to use CFLAGS1 to build file.c
CC=gcc
CFLAGS= -Wall -DPOSIX -g

all: all-lib all-exe

all-lib: libbar.a libmut.a libcond.a 

all-exe: bar mut cond

libbar.a: tst-barrier1.o tst-barrier3.o tst-barrier4.o 
	$(AR) -q $@ $^

libmut.a: tst-mutex1.o tst-mutex2.o tst-mutex3.o tst-mutex6.o tst-mutex7.o tst-mutex8.o
	$(AR) -q $@ $^

libcond.a: tst-cond2.o tst-cond3.o tst-cond7.o tst-cond10.o tst-cond14.o tst-cond16.o tst-cond18.o tst-cond22.o
	$(AR) -q $@ $^

bar:
	gcc -o bar tst-barrier.c libbar.a -lpthread $(CFLAGS)
mut:
	gcc -o mut tst-mutex.c libmut.a -lpthread $(CFLAGS)
cond:
	gcc -o cond tst-cond.c libcond.a -lpthread $(CFLAGS)



clean:
	rm -f *.o libbar.a libmut.a libcond.a bar mut cond
