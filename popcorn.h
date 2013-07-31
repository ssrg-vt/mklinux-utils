// Antonio Barbalace, SSRG, VT, 2013

/*
 * temporary utility function library
 * this library must be integrated in kexec (user/kernel space)
 */

#ifndef PAGE_SIZE
  #define PAGE_SIZE 0x1000
#endif

#define FILE_MEM "/dev/mem"

#define __NR_get_boot_params_addr 313

static inline unsigned long get_boot_params_addr () {
  return syscall(__NR_get_boot_params_addr);
}

#define MAX_RAMDISK_SIZE \
		((1UL << (unsigned long)(sizeof(boot_params_ptr->hdr.ramdisk_size) * 8)) -1)

//from arch/x86/boot/header.S
#define SETUP_HDR_SIGN  "HdrS"
#define SETUP_HDR_MINVER 0x0202
//from __cmdline_find_option
#define MAX_CMD_LINE_PTR 0x100000

#define ACTION_LOAD 0x000d
#define ACTION_SAVE 0x000e

int query_boot_params (struct boot_params * bootp, int action);

/*
 * load_boot_params
 * copies the current boot parameters from the running kernel
 * @param bootp allocated by the caller
 * @return 0 if success and 1 in case of error
 */
static inline int load_boot_params (struct boot_params * bootp) {
  return query_boot_params(bootp, ACTION_LOAD);
}

/*
 * save_boot_params
 * copies the passed boot parameters into the running kernel boot_params
 * @param bootp allocated and filled by the caller
 * @return 0 if success and 1 in case of error
 */
static inline int save_boot_params (struct boot_params * bootp) {
  return query_boot_params(bootp, ACTION_SAVE);
}

int query_cmd_line (struct boot_params * bootp, char * buffer, int * buf_size, int action);

/*
 * load_cmd_line
 * load the current cmd line parameters from boot_param structure
 * of the currently running kernel (or last booted in multi kernel)
 * @param previously popolated structure (use load_boot_params)
 * @param buffer buffer to copy the read cmd line 
 * @param buf_size buffer size in bytes (pointer to, will be populated with the count)
 * @retuen 0 if success 1 in case of error
 */
static inline int load_cmd_line(struct boot_params * bootp, char * buffer, int * buf_size) {
  return query_cmd_line(bootp, buffer, buf_size, ACTION_LOAD);
}

/*
 * save_cmd_line
 * save the passed by argument cmd line parameters to the boot_param structure
 * of the currently running kernel (or last booted in multi kernel)
 * @param previously popolated structure (use load_boot_params)
 * @param buffer buffer to copy the read cmd line 
 * @param buf_size buffer size in bytes (pointer to, will be populated with the count)
 * @retuen 0 if success 1 in case of error
 */
static inline int save_cmd_line(struct boot_params * bootp, char * buffer, int * buf_size) {
  return query_cmd_line(bootp, buffer, buf_size, ACTION_SAVE);
}


