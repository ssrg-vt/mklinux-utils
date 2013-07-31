// Antonio Barbalace, SSRG, VT, 2013

//#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>

#include "bootparam.h"
#include "popcorn.h"
 
int query_boot_params (struct boot_params * bootp, int action)
{
  int mem_fd, mem_flags, mem_prot, _ret;
  void* boot_params_phys_addr;
  void* boot_params_phys_page;
  void* boot_params_virt_page; 
  unsigned long boot_params_offset;
  struct boot_params * boot_params_ptr;
  
  if (!bootp) {
    printf("Error invalid arguments\n");
    return 1;
  }
  if ( !(action == ACTION_LOAD || action == ACTION_SAVE) ) {
    printf("Error invalid arguments, action\n");
    return 1;
  }
  
  /* Need to make a syscall to figure out where the boot_params struct is */
  boot_params_phys_addr = (void*) get_boot_params_addr();
  if ((boot_params_phys_addr == NULL) || (unsigned long)boot_params_phys_addr == ~0) {
    printf("Error calling get_boot_params_addr syscall (%ld)\n",
	   (unsigned long)boot_params_phys_addr);
    return 1;
  }
  
  boot_params_phys_page = (void*)((unsigned long)boot_params_phys_addr & ~(PAGE_SIZE -1));
  boot_params_offset = (unsigned long)boot_params_phys_addr & (PAGE_SIZE -1);
#ifdef DEBUG 
  printf("%s %s\n"
	 "boot_params_phys_addr %p boot_params_size %d\n"
	 "boot_params_phys_page %p boot_params_offset 0x%lx\n",
	 __func__,
	 (action == ACTION_LOAD) ? "RDONLY" : (action == ACTION_SAVE) ? "WRONLY" : "none",
	 boot_params_phys_addr, (int)sizeof(struct boot_params),
	 boot_params_phys_page, boot_params_offset);
#endif
  
  /* Open /dev/mem and mmap the boot arguments */
  mem_flags = (action == ACTION_LOAD) ? O_RDONLY :
     (action == ACTION_SAVE) ? O_RDWR : ~0; //~0 must generate a bug
  mem_fd = open(FILE_MEM, mem_flags);
  if (mem_fd == ~0) {
    //printf("Error. Failed to open %s!\n", FILE_MEM);
    perror("Error opening " FILE_MEM);
    return 1;
  }

  /* mapping of boot params struct in user space */
  mem_prot = (action == ACTION_LOAD) ? PROT_READ :
     (action == ACTION_SAVE) ? (PROT_WRITE | PROT_READ) : ~0; //~0 must generate a bug
  boot_params_virt_page = mmap(NULL, 2 * sizeof(struct boot_params),
			       mem_prot, MAP_SHARED,
			       mem_fd, (unsigned long)boot_params_phys_page);
  if (boot_params_virt_page == MAP_FAILED) {
    //printf("Error. Failed to mmap\n");
    perror("Error memory mapping " FILE_MEM);
    close(mem_fd);
    return 1;
  }
  boot_params_ptr = boot_params_virt_page + boot_params_offset;
#ifdef DEBUG 
  printf("boot_params_virt_page %p boot_params_ptr %p\n",
	 boot_params_virt_page, boot_params_ptr);
#endif  
  
  if (action == ACTION_LOAD)
    /* memcpy the boot_params struct to the user provided memory area */
    memcpy(bootp, boot_params_ptr, sizeof(struct boot_params));
  else if (action == ACTION_SAVE)
    /* memcpy the user provided memory boot_params in kernel */
    memcpy(boot_params_ptr, bootp, sizeof(struct boot_params));
  else {
    printf("bogus situation, action can not be different from *_LOAD and *_SAVE here\n");
    munmap(boot_params_virt_page, 2 * sizeof(struct boot_params));
    close(mem_fd);
    return 1;
  } 
    
  /* unmap the mapped region and return back resources */
  _ret = munmap(boot_params_virt_page, 2 * sizeof(struct boot_params));
  if (_ret) {
    printf("Error. Failed to munmap (%d)\n", _ret);  
    close(mem_fd);
    return 1;
  }
  close (mem_fd);
  return 0;
}

int query_cmd_line (struct boot_params * bootp, char * buffer, int * buf_size, int action)
{
  int mem_fd, mem_flags, mem_prot, _ret;
  void* cmd_line_phys_addr;
  void* cmd_line_phys_page;
  void* cmd_line_virt_page; 
  unsigned long cmd_line_offset;
  int cmd_line_size, cmd_line_strlen;
  struct setup_header * shdr;
  char * cmd_line_ptr;
  
  /* check the input arguments */
  if (!bootp || !buffer || !buf_size) {
    printf("Error invalid arguments\n");
    return 1;
  }
  if (*buf_size <1) {
    printf("Error invalid arguments, buffer size must be strictly positive\n");
    return 1;
  }
  if ( !(action == ACTION_LOAD || action == ACTION_SAVE) ) {
    printf("Error invalid arguments, action\n");
    return 1;
  }
    
  /* check the setup header signature and version */
  shdr = &bootp->hdr;
  if (memcmp(&shdr->header, SETUP_HDR_SIGN, sizeof(__u32))) {
    printf("Error invalid setup_header signature field %.4s (%.4s)\n",
      (char*)&shdr->header, SETUP_HDR_SIGN);
    return 1;
  }
  if (shdr->version < (__u16)SETUP_HDR_MINVER) {
    printf("Error setup_header version too old 0x%x (0x%x)\n",
      (unsigned int)shdr->version, (unsigned int)SETUP_HDR_MINVER);
    return 1;
  }
  
  /* get the address, and then the size, of the cmd line buffer */
  cmd_line_phys_addr = (void*)(unsigned long)shdr->cmd_line_ptr;
  if (!cmd_line_phys_addr ||
    (unsigned long)cmd_line_phys_addr > MAX_CMD_LINE_PTR) {
    printf("Error cmd_line inaccessible (%p)\n",
	   cmd_line_phys_addr);
    return 1;
  }
  cmd_line_size = shdr->cmdline_size;
  if (!cmd_line_size) {
    printf("Error cmd line size is zero\n");
    return 1;
  }
  
  cmd_line_phys_page = (void*)((unsigned long)cmd_line_phys_addr & ~(PAGE_SIZE -1));
  cmd_line_offset = (unsigned long)cmd_line_phys_addr & (PAGE_SIZE -1);
#ifdef DEBUG
  printf("%s %s\n"
	 "cmd_line_phys_addr %p cmd_line_size %d\n"
	 "cmd_line_phys_page %p cmd_line_offset 0x%lx\n",
	 __func__,
	 (action == ACTION_LOAD) ? "RDONLY" : (action == ACTION_SAVE) ? "WRONLY" : "none",
	 cmd_line_phys_addr, cmd_line_size,
	 cmd_line_phys_page, cmd_line_offset);
#endif
  
  /* Open /dev/mem and mmap the boot arguments */
  mem_flags = (action == ACTION_LOAD) ? O_RDONLY :
     (action == ACTION_SAVE) ? O_RDWR : ~0; //~0 must generate a bug
  mem_fd = open(FILE_MEM, mem_flags);
  if (mem_fd == ~0) {
    //printf("Error. Failed to open %s!\n", FILE_MEM);
    perror("Error opening " FILE_MEM);
    return 1;
  }

  /* mapping of boot params struct in user space */
  mem_prot = (action == ACTION_LOAD) ? PROT_READ :
     (action == ACTION_SAVE) ? (PROT_WRITE | PROT_READ) : ~0; //~0 must generate a bug
  cmd_line_virt_page = mmap(NULL, (cmd_line_size + cmd_line_offset),
			       mem_prot, MAP_SHARED,
			       mem_fd, (unsigned long)cmd_line_phys_page);
  if (cmd_line_virt_page == MAP_FAILED) {
    //printf("Error. Failed to mmap\n");
    perror("Error memory mapping " FILE_MEM);
    close(mem_fd);
    return 1;
  }
  cmd_line_ptr = cmd_line_virt_page + cmd_line_offset;
  cmd_line_strlen = strnlen(cmd_line_ptr, cmd_line_size);
#ifdef DEBUG 
  printf("cmd_line_virt_page %p cmd_line_ptr %p\n"
	 "cmd_line_strlen %d PAGE_SIZE 0x%x\n",
	 cmd_line_virt_page, cmd_line_ptr,
	 cmd_line_strlen, (unsigned int)PAGE_SIZE);
#endif  
  
  if (action == ACTION_LOAD) {
    if (*buf_size > cmd_line_strlen)
      *buf_size = cmd_line_strlen;
    /* memcpy the boot_params struct to the user provided memory area */
    memcpy(buffer, cmd_line_ptr, *buf_size);
  }
  else if (action == ACTION_SAVE) {
    if (*buf_size > cmd_line_strlen)
      *buf_size = cmd_line_strlen;
    /* first remove the previous content then memcpy */
    memset(cmd_line_ptr, 0, cmd_line_size);
    memcpy(cmd_line_ptr, buffer, *buf_size);
  }  
  else {
    printf("bogus situation, action can not be different from *_LOAD and *_SAVE here\n");
    munmap(cmd_line_virt_page, (cmd_line_size + cmd_line_offset));
    close(mem_fd);
    return 1;
  }   
      
  /* unmap the mapped region and return back resources */
  _ret = munmap(cmd_line_virt_page, (cmd_line_size + cmd_line_offset));
  if (_ret) {
    printf("Error. Failed to munmap (%d)\n", _ret);  
    close(mem_fd);
    return 1;
  }
  close (mem_fd);
  return 0;
}

// TODO
// TODO
// TODO
// todo but must return a linked list of system ram
/*
int system_ram()
{
  int ok = 0;
  size_t len = 0;
  char *line = NULL;
  long long total = 0;
  long long end = -1;
  long long start = -1;
  FILE *f; 
  int im = 0;

  memset(amemres, 0, sizeof(memres) * MAX_AMEMRES);

  f = fopen("/proc/iomem", "r");
  if (!f)
    return -1; 
  while (getdelim(&line, &len, '\n', f) > 0) { 
    char *endptr;
    char *s = strcasestr(line, "System RAM"); 
    if (!s) 
      continue;
    --s; 
    while (s > line && isspace(*s)) // space
      --s;
    while (s > line && ispunct(*s)) // column
      --s;
    while (s > line && isspace(*s)) // space
      --s;
    while (s > line && isxdigit(*s)) // hex end address
      --s; 

    end = strtoull((s +1),&endptr,16);
    if (endptr == s) 
      end = -1;
    else
      ok++; 

    while (s > line && ispunct(*s)) // column
      --s;
    while (s > line && isxdigit(*s)) // hex end address
      --s; 

    start = strtoull((s > line) ? (s +1) : s,&endptr,16);
    if (endptr == s) 
      end = -1;
    else
      ok++; 

    end++;
    total += (end -start);
		
#define ALIGN4KB
#ifdef ALIGN4KB
		start &= ~0x0FFF;
		end &= ~0x0FFF;
#endif
	
    //add to list
		amemres[im].start = start; 
		amemres[im].end = end;
		im++;
		printf("start %llx end %llx amount %lld B 0x%llx\n", start, end, (end -start), (end -start));
	} 
  fclose(f); 
  free(line);
  printf("total %lld %lld kB %lld MB %lld GB\n", total, total >>10, total >>20, total >>30);
	
  maxpresentmem = total;	
  return 0;
}
*/
