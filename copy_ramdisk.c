
// copyright Antonio Barbalace, SSRG, VT, 2013
// initial implementation copyright Ben Shelton, SSRG, VT, 2013

/*
 * this program will be integrated in kexec (user/kernel code)
 */

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bootparam.h"
#include "popcorn.h"

#define PATH_SIZE 1024

//input: ramdisk file ramdisk img_addr
int main(int argc, char *argv[])
{
	int mem_fd;
	uint64_t ramdisk_phys_addr = 0x60000000;
	uint64_t boot_params_phys_addr = 0x0;
	uint64_t boot_params_page, boot_params_offset;
	void *ramdisk_base_addr, *boot_params_page_base_addr;
	
	struct boot_params * boot_params_ptr;
	
	uint64_t ramdisk_size, size_read; 
  unsigned long phys_addr; or ramdisk_phys_addr
  char * filename, _len;
  FILE *ramdisk_fd;
	
	
  if (argc != 3) {
    printf("Usage: %s ADDR FILE\n"
	   "Copies the entire FILE at physical memory ADDR.\n",
	   argv[0]);
    return 1;
  }
  
  /*check each argument */
  phys_addr = strtoul(argv[1], 0, 0);
  if (phys_addr == 0) {
    perror("conversion error or physical address 0\n");
    return 1;
  }
  filename = malloc(PATH_SIZE);
  if (filename) {
    printf("malloc error\n");
    return 1;
  }
  strncpy(filename, argv[2], PATH_SIZE);
  _len = strlen(filename);
  if (_len == 0 || _len > PATH_SIZE) {
    printf("error in the file path\n");
    free(_cret);
    return 1;
  }
#ifdef DEBUG
  printf("ramdisk phys addr 0x%lx filename %s\n",
	 ramdisk_phys_addr, filename);
#endif
  
  /* check if the physical address is assigned to the current kernel */
//TODO code in popcorn lib
  
  RAMDISK PHYSICAL ADDRESS MUST BE PAGE ALIGNED!!!



///////////////////////////////////////////////////////////////////////////////

  /* Open the file with the ramdisk and determine its size */
  ramdisk_file = fopen(filename, "rb");
  if (!ramdisk_file) {
    printf("error fopen %s\n", filename);
    return 1;
  }
  fseek(ramdisk_file, 0, SEEK_END);
  ramdisk_size = ftell(ramdisk_file);
  fseek(ramdisk_file, 0, SEEK_SET);
#ifdef DEBUG
  printf("ramdisk size %lu maximum size %lu\n",
		  ramdisk_size,
		  ((1 << (sizeof((struct setup_header).ramdisk_size) * 8)) -1) );
#endif
  /* check if the ramdisk is too big */
  if ( ramdisk_size > 
    ((1 << (sizeof((struct setup_header).ramdisk_size) * 8)) -1) ) {
    printf ("error ramdisksize exceeds size limit (4GB)\n");
    fclose(ramdisk_file);
    return 1;
  }
  


	/* Open /dev/mem and mmap the boot arguments */
	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (mem_fd < 0) {
		printf("Failed to open /dev/mem!\n");
		return 1;
	}

	printf("Opened /dev/mem, fd %d\n", mem_fd);
	ramdisk_base_addr = mmap(0, ramdisk_size,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			mem_fd, ramdisk_phys_addr);
	printf("Ramdisk at 0x%lx, mapped addr 0x%lx\n", ramdisk_phys_addr, ramdisk_base_addr);

	/* Read the ramdisk into memory */
	size_read = fread(ramdisk_base_addr, 1, ramdisk_size, file);

	if (size_read != ramdisk_size) {
		printf("Failed to read the entire ramdisk into memory!\n");
		return 1;
	}

	munmap(ramdisk_base_addr, ramdisk_size);

	close(mem_fd);




  fclose(ramdisk_file);
  
///////////////////////////////////////////////////////////////////////////////

  /* load the boot parameters */
  boot_params_ptr = malloc(sizeof (struct boot_params));  
  if (!boot_params_ptr) {
    printf("malloc error boot_params\n");
    return 1;
  }
  
  _ret = load_boot_params (boot_params_ptr);
  if (_ret) {
    printf("library error getting struct\n");
    free(boot_params_ptr);
    return 1;
  }

  /* setting the ramdisk data in the setup header and saving them */
  boot_params_ptr->hdr.ramdisk_image = ramdisk_phys_addr & 0xffffffff;
  boot_params_ptr->hdr.ramdisk_shift = (ramdisk_phys_addr >> 32);
  boot_params_ptr->hdr.ramdisk_size = ramdisk_size;
  boot_params_ptr->hdr.ramdisk_magic = 0xdf;

  _ret = save_boot_params (boot_params_ptr);
  if (_ret) {
    printf("library error getting struct\n");
    free(boot_params_ptr);
    return 1;
  }
  
  printf("Boot params successfully set!\n");
  return 0;
}
