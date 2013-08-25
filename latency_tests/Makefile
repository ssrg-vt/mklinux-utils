all: test_kmsg test_ipi_latency client server

test_kmsg: test_kmsg.c
	cc -o $@ $<

test_ipi_latency: test_ipi_latency.c
	cc -o $@ $<

client: client.c
	cc -o $@ $<

server: server.c
	cc -o $@ $<


clean:
	rm -f client server test_kmsg test_ipi_latency *.o
