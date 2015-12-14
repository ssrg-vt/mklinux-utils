 
// Copyright (C) 2013 - 2014, Antonio Barbalace
// Popcorn heartbeat daemon
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/io.h>

#define _GNU_SOURCE
#include <utmpx.h>

static char buffer[128];

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

int main (int argc, char* argv[])
{

  int n;
  if (argc != 2) {
    printf("usage: heartbeat n, where n is the number of seconds between each beat.\n");
    exit(0);
  }
  else {
      char* end;
      int n = strtol(argv[1], &end, 10);
      if (end == argv[1]) {
          printf("usage: heartbeat n, where n is the number of seconds between each beat.\n");
          exit(0);
      }
      printf("heartbeat: beating every %d seconds\n", n);
  }


  pid_t pid= fork();
  if (pid <0) {
    printf("fork error. exit.\n");
    exit(1); // fork error
  }
  if (pid >0)
    exit(0); // parent exits

  pid_t sid =setsid();
  if (sid <0) {
    printf("setsid error, exit.\n");
    exit(1); //setsid error
  }

  chdir("/");

  close(0);
  close(1);
  close(2);

  int fd = open("/dev/null", O_RDWR, 0);
  if (fd != -1) {
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    if (fd > 2)
      close(fd);    
  }
  close(fd);
 
#ifdef USE_LINUX  
  fd = open("/dev/ttyS0", O_RDWR, 0);
  if (fd == -1) 
    exit(0);
  
  int i =0;
  while (1) {
    sprintf(buffer, "heartbeat core%d cnt%d\r\n", sched_getcpu(), i++);
    write(fd, buffer, strlen(buffer));
    fsync(fd);
    sleep (n);
  }
  close(fd);
#else
  if (ioperm(PORT, 6, 1)) {
    printf("Failed to get I/O permissions!\n");
    return 0;
  }
  init_serial();
  
  int i =0, l;
  while(1) {
    sprintf(buffer, "heartbeat core%d cnt%d\r\n", sched_getcpu(), i++);
    for (l = 0; l < strlen(buffer); l++) 
      write_serial(buffer[l]);
    sleep (15);
  }
#endif

  return 0;
}
