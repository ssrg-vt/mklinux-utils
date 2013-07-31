// copyright Antonio Barbalace, SSRG, VT, 2013

/*
 * read_cmdline
 * simple read (kernel) cmdline utility
 * note this IS NOT equivalent to /proc/cmdline
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bootparam.h"
#include "popcorn.h"

#define CMDLINE_SIZE 2048

int main(int argc, char *argv[])
{
  struct boot_params * bootp;
  int cmd_line_size;
  char * cmd_line_ptr;
  int _ret;
  
  bootp = malloc(sizeof (struct boot_params));  
  if (!bootp) {
    printf("malloc error boot_params\n");
    return 1;
  }
  
  _ret = load_boot_params (bootp);
  if (_ret) {
    printf("library error getting struct\n");
    free(bootp);
    return 1;
  }
    
  cmd_line_size = CMDLINE_SIZE;
  cmd_line_ptr = malloc(CMDLINE_SIZE);
  if (!cmd_line_ptr) {
    printf("malloc error cmd_line_ptr\n");
    free (bootp);
    return 1;
  }
  memset(cmd_line_ptr, 0, cmd_line_size);

  _ret = load_cmd_line (bootp, cmd_line_ptr, &cmd_line_size);
  if (_ret) {
    printf("library error getting cmd line\n");
    free(cmd_line_ptr);
    free(bootp);
    return 1;
  }
  
  //printf("CMDLINE(%d):\n%s\n", cmd_line_size, cmd_line_ptr);
  printf("%s\n", cmd_line_ptr);

  free(cmd_line_ptr);
  free(bootp);
  return 0;
}
