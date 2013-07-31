// copyright Antonio Barbalace, SSRG, VT, 2013
// partially based on initial work by Ben Shelton, SSRG, VT, 2013

/* write_cmdline
 * simple write (kernel) cmdline utility
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
  
  if (argc != 2) {
    printf("Usage: %s FILE\n"
	   "Copies the entire FILE at physical memory ADDR.\n",
	   argv[0]);
    return 1;
  }
  
  
  TODO TODO, proposal:
  wr_cmdline -f file.txt <- use this right now
  wr_cmdline "cmd line"
  
  
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
   
   
   pipe in or read a file
// TODO   populate the cmd line buffer before writing
   
  cmd_line_size = CMDLINE_SIZE;
  cmd_line_ptr = malloc(CMDLINE_SIZE);
  if (!cmd_line_ptr) {
    printf("malloc error cmd_line_ptr\n");
    free (bootp);
    return 1;
  }
  memset(cmd_line_ptr, 0, cmd_line_size);

  char ccmd[] = "mimmo is mimmo and you?";
  cmd_line_size = sizeof(ccmd);
  
  _ret = save_cmd_line (bootp, ccmd, &cmd_line_size);
  if (_ret) {
    printf("library error saving the cmd line\n");
    free(cmd_line_ptr);
    free(bootp);
    return 1;
  }
  printf("%s\n", ccmd);
  
  free(cmd_line_ptr);
  free(bootp);
  return 0;
}
