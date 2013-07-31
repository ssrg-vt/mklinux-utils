/* set_boot_args.c
 * 
 * this program set the boot arguments by accessing 
 * /dev/mem.
 *
 * WARNING this code must be considered *temporary*, it will be integrated in 
 * kexec (user and kernel)
 * 
 * initial version by Ben Shelton
 * copyright Ben Shelton, SSRG, VT, 2013
 * copyright Antonio Barbalace, SSRG, VT, 2013
 */

/* set_boot_args.c is used to set the boot arguments */

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
   
/*
 * CODE FROM arch/x86/boot/main.c
 static void copy_boot_params(void)
{
	struct old_cmdline {
		u16 cl_magic;
		u16 cl_offset;
	};
	const struct old_cmdline * const oldcmd =
		(const struct old_cmdline *)OLD_CL_ADDRESS;

	BUILD_BUG_ON(sizeof boot_params != 4096);
	memcpy(&boot_params.hdr, &hdr, sizeof hdr);

	if (!boot_params.hdr.cmd_line_ptr &&
	    oldcmd->cl_magic == OLD_CL_MAGIC) {
		// Old-style command line protocol.
		u16 cmdline_seg;

		// Figure out if the command line falls in the region
		// of memory that an old kernel would have copied up
		// to 0x90000...
		if (oldcmd->cl_offset < boot_params.hdr.setup_move_size)
			cmdline_seg = ds();
		else
			cmdline_seg = 0x9000;

		boot_params.hdr.cmd_line_ptr =
			(cmdline_seg << 4) + oldcmd->cl_offset;
	}
}
void main(void)
{
	// First, copy the boot header into the "zeropage"
	copy_boot_params();
*/
// boot_params.hdr.cmd_line_ptr
/*
int __cmdline_find_option(u32 cmdline_ptr, const char *option, char *buffer, int bufsize)
{
	addr_t cptr;
	char c;
	int len = -1;
	const char *opptr = NULL;
	char *bufptr = buffer;
	enum {
		st_wordstart,	// Start of word/after whitespace
		st_wordcmp,	// Comparing this word
		st_wordskip,	// Miscompare, skip
		st_bufcpy	// Copying this to buffer
	} state = st_wordstart;

	if (!cmdline_ptr || cmdline_ptr >= 0x100000)
		return -1;	// No command line, or inaccessible

	cptr = cmdline_ptr & 0xf;
	set_fs(cmdline_ptr >> 4);

	while (cptr < 0x10000) {
*/
//from arch/x86/boot/header.S
/*
cmd_line_ptr:	.long	0		# (Header version 0x0202 or later)
					# If nonzero, a 32-bit pointer
					# to the kernel command line.
					# The command line should be
					# located between the start of
					# setup and the end of low
					# memory (0xa0000), or it may
					# get overwritten before it
					# gets read.  If this field is
					# used, there is no longer
					# anything magical about the
					# 0x90000 segment; the setup
					# can be located anywhere in
					# low memory 0x10000 or higher.
 */
int main(int argc, char *argv[]) {

	int mem_fd;
	void * boot_args_base_addr;
	char boot_args[2048];

	static const char filename[] = "bootargs.txt";
	FILE *file;

	if (argc != 2) {
		printf("Invalid number of arguments specified!\n");
		return 1;
	}

	file = fopen(argv[1], "r");

	if (!file) {
		printf("Failed to open file containing boot args...\n");
		return 1;
	}

	/* Open the file with the boot args and read them in */
	if (!fgets(boot_args, sizeof(boot_args), file)) {
		printf("Failed to read in boot args from file!\n");
		return 1;
	}

	if (boot_args[strlen(boot_args) - 1] == '\n') {
		boot_args[strlen(boot_args) - 1] = '\0';
	}

	printf("Boot arguments: %s\n", boot_args);

	/* Open /dev/mem and mmap the boot arguments */
	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (mem_fd < 0) {
		printf("Failed to open /dev/mem!\n");
		return 1;
	}

	printf("Opened /dev/mem, fd %d\n", mem_fd);
	boot_args_base_addr = mmap(0, 0x800, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x8d000);
	printf("Boot args at 0x8d000, mapped addr 0x%lx\n", boot_args_base_addr);

	/* Copy the boot arguments to the right place */
	strcpy((char *)boot_args_base_addr, (char *)boot_args);

	return 0;
}
