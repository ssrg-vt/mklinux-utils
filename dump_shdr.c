// copyright Antonio Barbalace, SSRG, VT, 2013

/* dump_shdr
 * simple dump setup_header struct utility
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib/bootparam.h"
#include "lib/popcorn.h"

int main(int argc, char *argv[])
{
  struct boot_params * bootp;
  struct setup_header * shdr;
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
  shdr = &bootp->hdr;

  printf("---- setup_header dump ----\n"
	 "header         0x%x \"%.4s\"\n"
	 "version        0x%x\n"
	 "kernel_version 0x%x\n"
	 "type_of_loader 0x%x\n"
	 "code32_start   0x%x\n"
	 "ramdisk_image  0x%x\n"
	 "ramdisk_size   0x%x\n"
	 "cmd_line_ptr   0x%x\n"
	 "cmdline_size   0x%x\n"
	 "ramdisk_shift  0x%x\n"
	 "ramdisk_magic  0x%x\n",
	 (unsigned int) shdr->header, (char*) &shdr->header,
	 (unsigned int) shdr->version,
	 (unsigned int) shdr->kernel_version,
	 (unsigned int) shdr->type_of_loader,
	 (unsigned int) shdr->code32_start,
	 (unsigned int) shdr->ramdisk_image,
	 (unsigned int) shdr->ramdisk_size,
	 (unsigned int) shdr->cmd_line_ptr,
	 (unsigned int) shdr->cmdline_size,
	 (unsigned int) shdr->ramdisk_shift,
	 (unsigned int) shdr->ramdisk_magic);
  
  free(bootp);
  return 0;
}
