// copyright Antonio Barbalace, SSRG, VT, 2013
// partially based on initial work by Ben Shelton, SSRG, VT, 2013

/* write_cmdline
 * simple write (kernel) cmdline utility
 * note this IS NOT equivalent to /proc/cmdline
 *
 * TODO
 * instead of wr_cmdline filename adopt the following:
 * $ wr_cmdline -f file.txt
 * $ wr_cmdline "cmd line"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib/bootparam.h"
#include "lib/popcorn.h"

#define PATH_SIZE 1024

#define CMDLINE_SIZE 2048

int main(int argc, char *argv[])
{
  struct boot_params * bootp;
  char * filename;
  char * cmdline_ptr;
  int cmdline_size;
  int _ret, _len, size_read;
  FILE * cmdline_file;
  
  if (argc != 2) {
    printf("Usage: %s FILE\n"
	   "Copies the content of FILE to the cmd_line_ptr buffer in phys space.\n",
	   argv[0]);
    return 1;
  }
  
  /* check the args*/
  filename = malloc(PATH_SIZE);
  if (!filename) {
	  printf("malloc error for filename (%d)\n", PATH_SIZE);
	  return 1;
  }
  strncpy(filename, argv[1], PATH_SIZE);
  _len = strlen(filename);
  if (_len == 0 || _len > PATH_SIZE) {
    printf("error in the file path\n");
    free(filename);
    return 1;
  }

  /* Open the file with the ramdisk and determine its size */
  cmdline_file = fopen(filename, "rb");
  if (!cmdline_file) {
    printf("error fopen %s\n", filename);
    free(filename);
    return 1;
  }
  fseek(cmdline_file, 0, SEEK_END);
  cmdline_size = ftell(cmdline_file);
  fseek(cmdline_file, 0, SEEK_SET);

  /* load the struct boot_params from /dev/mem */
  bootp = malloc(sizeof (struct boot_params));  
  if (!bootp) {
    printf("malloc error boot_params\n");
    fclose(cmdline_file);
    free(filename);
    return 1;
  }

  /* read the cmdline into memory */
  cmdline_ptr = malloc(cmdline_size);
  if (!cmdline_ptr) {
    printf("malloc error cmd_line_ptr\n");
    free(bootp);
    fclose(cmdline_file);
    free(filename);
    return 1;
  }
  size_read = fread(cmdline_ptr, 1, cmdline_size, cmdline_file);
  if (size_read != cmdline_size) {
  	printf("Failed to read the entire cmdline into memory!\n");
  	free(cmdline_ptr);
    free(bootp);
    fclose(cmdline_file);
    free(filename);
  	return 1;
  }
  fclose(cmdline_file);
  free(filename);

  /* loading the command line in phys memory */
  _ret = load_boot_params (bootp);
  if (_ret) {
    printf("library error getting struct\n");
    free(bootp);
    return 1;
  }
  _ret = save_cmd_line (bootp, cmdline_ptr, &cmdline_size);
  if (_ret) {
    printf("library error saving the cmd line\n");
    free(cmdline_ptr);
    free(bootp);
    return 1;
  }
  
  free(cmdline_ptr);
  free(bootp);
  return 0;
}
