/* set_boot_args.c is used to set the boot arguments */

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "bootparam.h"

#define __NR_get_boot_params_addr 313

int main(int argc, char *argv[]) {

	int mem_fd;
	uint64_t ramdisk_phys_addr = 0x60000000;
	uint64_t boot_params_phys_addr = 0x0;
	uint64_t boot_params_page, boot_params_offset;
	void *ramdisk_base_addr, *boot_params_page_base_addr;
	struct boot_params *boot_params_base_addr;
	uint64_t ramdisk_size, size_read; 
	static const char filename[] = "ramdisk.img";
	FILE *file = fopen(filename, "rb");

	if (argc != 2) {
		printf("Invalid number of arguments specified!\n");
		return 0;
	}

	sscanf(argv[1], "%lx", &ramdisk_phys_addr);

	printf("Ramdisk phys addr is 0x%lx\n", ramdisk_phys_addr);

	if (!file) {
		printf("Failed to open file containing boot args...\n");
		return 0;
	}

	/* Open the file with the ramdisk and determine its size */
	fseek(file, 0, SEEK_END);
	ramdisk_size = ftell(file);
	fseek(file, 0, SEEK_SET);


	printf("Ramdisk size is %lu bytes\n", ramdisk_size);

	/* Open /dev/mem and mmap the boot arguments */
	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (mem_fd < 0) {
		printf("Failed to open /dev/mem!\n");
		return 0;
	}

	printf("Opened /dev/mem, fd %d\n", mem_fd);
	ramdisk_base_addr = mmap(0, ramdisk_size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, ramdisk_phys_addr);
	printf("Ramdisk at 0x%lx, mapped addr 0x%lx\n", ramdisk_phys_addr, ramdisk_base_addr);

	/* Read the ramdisk into memory */
	size_read = fread(ramdisk_base_addr, 1, ramdisk_size, file);

	if (size_read != ramdisk_size) {
		printf("Failed to read the entire ramdisk into memory!\n");
		return 0;
	}

	munmap(ramdisk_base_addr, ramdisk_size);

	close(mem_fd);

	printf("Read ramdisk into memory; setting boot params...\n");

	/* Need to make a syscall to figure out where the boot_params struct is */
	boot_params_phys_addr = syscall(__NR_get_boot_params_addr);

	boot_params_page = boot_params_phys_addr & 0xfffffffffffff000ULL;
	boot_params_offset = boot_params_phys_addr & 0xfff;

	printf("boot_params_page 0x%lx, boot_params_offset 0x%lx\n", 
			boot_params_page, boot_params_offset);

	/* Open /dev/mem and mmap the boot arguments */
	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (mem_fd < 0) {
	        printf("Failed to open /dev/mem!\n");
	        return 0;
	}

	printf("Opened /dev/mem, fd %d\n", mem_fd);

	boot_params_page_base_addr = mmap(0, 2 * sizeof(struct boot_params), 
					PROT_READ | PROT_WRITE, MAP_SHARED, 
					mem_fd, boot_params_page);

	boot_params_base_addr = boot_params_page_base_addr + boot_params_offset;

	printf("Boot params at 0x%lx, mapped addr 0x%lx\n", boot_params_phys_addr, boot_params_base_addr);


	boot_params_base_addr->hdr.ramdisk_image = ramdisk_phys_addr & 0xffffffff;

	boot_params_base_addr->hdr.ramdisk_shift = (ramdisk_phys_addr >> 32);

	boot_params_base_addr->hdr.ramdisk_size = ramdisk_size;

	printf("Boot params successfully set!\n");

	return 0;
}
