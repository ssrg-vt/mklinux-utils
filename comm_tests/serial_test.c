/* This is a serial test program that writes directly to a serial port.
   Code comes in part from wiki.osdev.org/Serial_Ports
   Note that this must be run as root! */

#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>
#include <string.h>

#define PORT 0x3f8   /* /dev/ttyS0 */
 
void init_serial() {
	outb(0x00, PORT + 1);    // Disable all interrupts
	outb(0x80, PORT + 3);    // Enable DLAB (set baud rate divisor)
	outb(0x01, PORT + 0);    // Set divisor to 1 (lo byte) 115200 baud
	outb(0x00, PORT + 1);    //                  (hi byte)
	outb(0x03, PORT + 3);    // 8 bits, no parity, one stop bit
	outb(0xC7, PORT + 2);    // Enable FIFO, clear them, with 14-byte threshold
	outb(0x0B, PORT + 4);    // IRQs enabled, RTS/DSR set
}


int is_transmit_empty() {
	return inb(PORT + 5) & 0x20;
}
 
void write_serial(char a) {
	while (is_transmit_empty() == 0);    
	outb(a, PORT);
}

int main(int argc, char *argv[]) {
	
	char test_string[] = "This is a test of the serial port.\n";
	int i;
	
	if (ioperm(PORT, 6, 1)) {
		printf("Failed to get I/O permissions!\n");
		return 0;
	}

	printf("Initializing serial port...\n");
	init_serial();

	printf("Writing a string...\n");
	for (i = 0; i < strlen(test_string); i++) {
		write_serial(test_string[i]);
	}

	printf("All done!\n");

	return 0;
}
