#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#define KMEM_PATH "/dev/mem"

int main (int argc, char * argv[]) {

 unsigned long long begin_paddr= atoll(argv[1]);
 unsigned long long size= atoll(argv[2]);
 unsigned long long end_paddr= begin_paddr+size;
 //end_paddr= atoi(argv[2]);

 int kcfd = open(KMEM_PATH, O_RDONLY);
   if (kcfd == -1) {
    perror("kcfd error file creation\n");
    return 0;
  }

lseek(kcfd, begin_paddr, SEEK_SET);

char * buffer = malloc(4096);
if ( !buffer)
  goto outside;
while (begin_paddr < end_paddr) {
ssize_t rrr = read (kcfd, buffer, 4096);
printf("%s", buffer);
begin_paddr +=4096;
}

outside:
  close(kcfd);
return 0;
}

