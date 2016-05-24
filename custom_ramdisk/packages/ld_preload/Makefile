CFLAGS  = -fPIC -Wall -Wextra -shared -g
LDFLAGS = -ldl -lpthread

all: pcn_det.so pthread_test

pcn_det.so:
	gcc $(CFLAGS) -o pcn_det.so pcn_det.c $(LDFLAGS)

pthread_test:
	gcc pthread_test.c -o pthread_test $(LDFLAGS)

clean:
	rm -rf pcn_det.so pthread_test

install:
	cp pcn_det.so /usr/lib
