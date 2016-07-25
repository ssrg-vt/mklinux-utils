CC=gcc
CFLAGS=-Wall -Werror
LDFLAGS=
BINS=get_affinity ns_exec simple_init ns_child_exec

SRC="get_affinity.c ns_exec.c simple_init.c unshare.c ns_child_exec.c ns_exec_orig.c simple_init_orig.c"

all: $(BINS)

get_affinity: get_affinity.c
	gcc $(CFLAGS) $^ -o $@ $(LDFLAGS)

ns_exec: ns_exec.c
	gcc $(CFLAGS) $^ -o $@ $(LDFLAGS)
	
simple_init: simple_init.c
	gcc $(CFLAGS) $^ -o $@ $(LDFLAGS)

ns_child_exec: ns_child_exec.c
	gcc $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf *.o $(BINS)
