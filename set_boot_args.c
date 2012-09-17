/* set_boot_args.c is used to set the boot arguments */

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {

	int mem_fd;
	void * boot_args_base_addr;
	char boot_args[2048];

	static const char filename[] = "bootargs.txt";
	FILE *file;

	if (argc != 2) {
		printf("Invalid number of arguments specified!\n");
		return 0;
	}

	file = fopen(argv[1], "r");

	if (!file) {
		printf("Failed to open file containing boot args...\n");
		return 0;
	}

	/* Open the file with the boot args and read them in */
	if (!fgets(boot_args, sizeof(boot_args), file)) {
		printf("Failed to read in boot args from file!\n");
		return 0;
	}

	if (boot_args[strlen(boot_args) - 1] == '\n') {
		boot_args[strlen(boot_args) - 1] = '\0';
	}

	printf("Boot arguments: %s\n", boot_args);

	/* Open /dev/mem and mmap the boot arguments */
	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (mem_fd < 0) {
		printf("Failed to open /dev/mem!\n");
		return 0;
	}

	printf("Opened /dev/mem, fd %d\n", mem_fd);
	boot_args_base_addr = mmap(0, 0x800, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x20000);
	printf("Boot args at 0x20000, mapped addr 0x%lx\n", boot_args_base_addr);

	/* Copy the boot arguments to the right place */
	strcpy((char *)boot_args_base_addr, (char *)boot_args);

	return 0;
}
