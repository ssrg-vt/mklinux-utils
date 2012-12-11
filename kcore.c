
#define _LARGEFILE64_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <linux/elf.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>



#define	EI_MAG0		0		/* e_ident[] indexes */
#define	EI_MAG1		1
#define	EI_MAG2		2
#define	EI_MAG3		3
#define	EI_CLASS	4
#define	EI_DATA		5
#define	EI_VERSION	6
#define	EI_OSABI	7
#define	EI_PAD		8

#define	ELFMAG0		0x7f		/* EI_MAG */
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

#define	ELFCLASSNONE	0		/* EI_CLASS */
#define	ELFCLASS32	1
#define	ELFCLASS64	2
#define	ELFCLASSNUM	3

#define ELFDATANONE	0		/* e_ident[EI_DATA] */
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

#define EV_NONE		0		/* e_version, EI_VERSION */
#define EV_CURRENT	1
#define EV_NUM		2

#define ELFOSABI_NONE	0
#define ELFOSABI_LINUX	3

#ifndef ELF_OSABI
#define ELF_OSABI ELFOSABI_NONE
#endif

#define ELF_CLASS	ELFCLASS64
#define ELF_DATA	ELFDATA2LSB
#define ELF_ARCH	EM_X86_64
#define ELF_CORE_EFLAGS 0

#define elfhdr		elf64_hdr
#define elf_phdr	elf64_phdr
#define elf_shdr	elf64_shdr
#define elf_note	elf64_note
#define elf_addr_t	Elf64_Off
#define Elf_Half	Elf64_Half


enum kcore_type {
	KCORE_TEXT,
	KCORE_VMALLOC,
	KCORE_RAM,
	KCORE_VMEMMAP,
	KCORE_OTHER,
};

struct kcore_list {
//	struct list_head list;
	unsigned long addr;
	unsigned long virtual;
	unsigned long file_off;
	size_t size;
	int type;
};

#define PAGE_SIZE 0x1000
#define MAX_SEGMENTS 4

//static map for kernel 0
//must be statically generated from /dev/ioremap
/*struct kcore_list klist[MAX_SEGMENTS] = {{ .addr=0x00010000, .virtual=0xffff880000010000, .size=581632, .type=KCORE_RAM},
			    { .addr=0x00100000, .virtual=0xffff880000100000, .size=3621126144, .type=KCORE_RAM},
			    //{ .addr=0x100000000, .virtual=0xffff880100000000, .size=133798297600, .type=KCORE_RAM},
			    { .addr=0x100000000, .virtual=0xffff880100000000, .size=0x100000000, .type=KCORE_RAM},
			    { .addr=0x01000000, .virtual=0xffffffff81000000, .size=11108352, .type=KCORE_TEXT}};
*/
//unsigned long phy_base_offset = 0;
unsigned long phy_base_offset = 0x240000000;
struct kcore_list klist[MAX_SEGMENTS] = {{ .addr=0x00010000, .virtual=0xffff880000010000, .size=581632, .type=KCORE_RAM},
                            { .addr=0x00100000, .virtual=0xffff880000100000, .size=0xf00000, .type=KCORE_RAM},
                            //{ .addr=0x100000000, .virtual=0xffff880100000000, .size=133798297600, .type=KCORE_RAM},
                            { .addr=0x240000000, .virtual=0xffff880240000000, .size=0x200000000, .type=KCORE_RAM},
                            { .addr=0x240000000, .virtual=0xffffffff81000000, .size=0x2000000, .type=KCORE_TEXT}};

/*
 * store an ELF coredump header in the supplied buffer
 * nphdr is the number of elf_phdr to insert
 */
static void elf_kcore_store_hdr(char *bufp, int nphdr, int dataoff)
{
//	struct elf_prstatus prstatus;	/* NT_PRSTATUS */
//	struct elf_prpsinfo prpsinfo;	/* NT_PRPSINFO */
	struct elf_phdr *nhdr, *phdr;
	struct elfhdr *elf;
//	struct memelfnote notes[3];
	off_t offset = 0;
//	struct kcore_list *m;

	/* setup ELF header */
	elf = (struct elfhdr *) bufp;
	bufp += sizeof(struct elfhdr);
	offset += sizeof(struct elfhdr);
	
	memcpy(elf->e_ident, ELFMAG, SELFMAG);
	elf->e_ident[EI_CLASS]	= ELF_CLASS;
	elf->e_ident[EI_DATA]	= ELF_DATA;
	elf->e_ident[EI_VERSION]= EV_CURRENT;
	elf->e_ident[EI_OSABI] = ELF_OSABI;
	memset(elf->e_ident+EI_PAD, 0, EI_NIDENT-EI_PAD);
	elf->e_type	= ET_CORE;
	elf->e_machine	= ELF_ARCH;
	elf->e_version	= EV_CURRENT;
	elf->e_entry	= 0;
	elf->e_phoff	= sizeof(struct elfhdr);
	elf->e_shoff	= 0;
	elf->e_flags	= ELF_CORE_EFLAGS;
	elf->e_ehsize	= sizeof(struct elfhdr);
	elf->e_phentsize= sizeof(struct elf_phdr);
	elf->e_phnum	= nphdr +1;
	elf->e_shentsize= 0;
	elf->e_shnum	= 0;
	elf->e_shstrndx	= 0;

	/* setup ELF PT_NOTE program header */
	nhdr = (struct elf_phdr *) bufp;
	bufp += sizeof(struct elf_phdr);
	offset += sizeof(struct elf_phdr);
	
	nhdr->p_type	= PT_NOTE;
	nhdr->p_offset	= 0;
	nhdr->p_vaddr	= 0;
	nhdr->p_paddr	= 0;
	nhdr->p_filesz	= 0;
	nhdr->p_memsz	= 0;
	nhdr->p_flags	= 0;
	nhdr->p_align	= 0;

	/* setup ELF PT_LOAD program header for every area */
//	list_for_each_entry(m, &kclist_head, list) {
	int i;
	for (i=0; i<nphdr; i++) {
		phdr = (struct elf_phdr *) bufp;
		bufp += sizeof(struct elf_phdr);
		offset += sizeof(struct elf_phdr);

		phdr->p_type	= PT_LOAD;
		phdr->p_flags	= PF_R|PF_W|PF_X;
		phdr->p_offset	= klist[i].file_off + dataoff;
		phdr->p_vaddr	= (size_t)klist[i].virtual;
		phdr->p_paddr	= (size_t)klist[i].addr; // maybe 0
		phdr->p_filesz	= phdr->p_memsz	= klist[i].size;
		phdr->p_align	= PAGE_SIZE;
	}

	/*
	 * Set up the notes in similar form to SVR4 core dumps made
	 * with info from their /proc.
	 */
/*	
	nhdr->p_offset	= offset;

// set up the process status
	notes[0].name = CORE_STR;
	notes[0].type = NT_PRSTATUS;
	notes[0].datasz = sizeof(struct elf_prstatus);
	notes[0].data = &prstatus;

	memset(&prstatus, 0, sizeof(struct elf_prstatus));

	nhdr->p_filesz	= notesize(&notes[0]);
	bufp = storenote(&notes[0], bufp);

// set up the process info
	notes[1].name	= CORE_STR;
	notes[1].type	= NT_PRPSINFO;
	notes[1].datasz	= sizeof(struct elf_prpsinfo);
	notes[1].data	= &prpsinfo;

	memset(&prpsinfo, 0, sizeof(struct elf_prpsinfo));
	prpsinfo.pr_state	= 0;
	prpsinfo.pr_sname	= 'R';
	prpsinfo.pr_zomb	= 0;

	strcpy(prpsinfo.pr_fname, "vmlinux");
	strncpy(prpsinfo.pr_psargs, saved_command_line, ELF_PRARGSZ);

	nhdr->p_filesz	+= notesize(&notes[1]);
	bufp = storenote(&notes[1], bufp);

// set up the task structure
	notes[2].name	= CORE_STR;
	notes[2].type	= NT_TASKSTRUCT;
	notes[2].datasz	= sizeof(struct task_struct);
	notes[2].data	= current;

	nhdr->p_filesz	+= notesize(&notes[2]);
	bufp = storenote(&notes[2], bufp);
	*/
} /* end elf_kcore_store_hdr() */

#define SEG_ALIGNMENT PAGE_SIZE
// the calculated offset is percalulated based on the end of the headers (i.e. must be summed before writing the header)
void calcolate_offs()
{
  int i;
  unsigned long off = 0;
  unsigned long toff = 0;
  
  for (i=0; i<MAX_SEGMENTS; i++) {
    klist[i].file_off = off;
    if (toff = klist[i].size & (SEG_ALIGNMENT -1))
      toff = SEG_ALIGNMENT - toff;
    off += (klist[i].size + toff);
  }
  
}

#define FILE_PATH "vmlinux.kcore"
#define KMEM_PATH "/dev/mem"

int main (int argc, char * argv[])
{
//  adjust_klist();
  calcolate_offs();

  int nphdr = MAX_SEGMENTS;
  size_t elf_off, elf_buflen = sizeof(struct elfhdr) + 			// elf header
		      (nphdr + 1)*sizeof(struct elf_phdr); 		// elf p header (nphdr +2) "program header"
  elf_off = elf_buflen & (PAGE_SIZE -1);
  elf_buflen = (elf_off) ? (elf_buflen + PAGE_SIZE - elf_off): elf_buflen;
  printf("elf_buflen %ld\n", elf_buflen);      
  
  char * bufp = malloc(elf_buflen); // alloc header buffer
  if (!bufp) {
    printf("malloc error\n");
    return -1;
  } 
  memset(bufp, 0, elf_buflen);
  elf_kcore_store_hdr(bufp, nphdr, elf_buflen);
  
  int fd = open(FILE_PATH, O_RDWR | O_CREAT,
	(S_IRWXU | S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRGRP | S_IWGRP | S_IXGRP | S_IRWXO));
  if (fd == -1) {
    perror("fd error file creation\n");
    return 0;
  }
  write(fd, bufp, elf_buflen);
  free(bufp);
  
  char * kcbuffer = malloc(PAGE_SIZE);
  int kcfd = open(KMEM_PATH, O_RDONLY);
  if (kcfd == -1) {
    perror("kcfd error file creation\n");
    return 0;
  }
  
  // let's read each area from the kernel physical mapping!
  int i;
  for (i=0; i<MAX_SEGMENTS; i++) {
    ssize_t __ret;
    if (klist[i].type == KCORE_RAM) {
      __ret = lseek64 (kcfd, klist[i].addr, SEEK_SET); //physical addr
      if (__ret == -1)
	perror("kcfd lseek error\n");
    }
    else if (klist[i].type == KCORE_TEXT) {
      __ret = lseek64 (kcfd, klist[i].addr, SEEK_SET); //physical addr
      if (__ret == -1)
	perror("kcfd lseek error\n");
    }
    else
      continue;

    printf("kcfd @ 0x%lx\n", __ret);
    printf("klist addr 0x%lx size %ld\n", klist[i].addr, klist[i].size);
    unsigned long  count = 0;
    while ((unsigned long)(count * PAGE_SIZE) < (unsigned long)klist[i].size) {
      ssize_t _ret = read(kcfd, kcbuffer, PAGE_SIZE );
      if (_ret != PAGE_SIZE)
	//printf("read returns %d\n", (int) _ret);
	perror("kcfd read error\n");

      _ret = write(fd, kcbuffer, PAGE_SIZE); // paddings?
      if (_ret != PAGE_SIZE)
	//printf("write returns %d\n", (int) _ret);
        perror("fd write error\n");
      
      count ++;
    }
    printf (" count %lu\n", count);
  }
  
  close(kcfd);
  
  close(fd);  
  return 0;
}

