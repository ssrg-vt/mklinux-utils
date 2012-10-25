/* this is launch_xml.c, a parser to perform MKLinux launch */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "launch_xml.h"

#define DEBUG

struct common_params my_common_params;
struct guest_kernel_list *list_head = NULL, *list_elt = NULL;

void
parseOverall (xmlDocPtr doc, xmlNodePtr cur) {

	xmlChar *key;
	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"lapic_timer"))) {
  			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			printf("lapic_timer: %s\n", key);
			my_common_params.lapic_timer = strtoul(key, NULL, 0);
    			xmlFree(key);
     		}
    		cur = cur->next;
	}

	return;
}

void
parseKernel (xmlDocPtr doc, xmlNodePtr cur) {

	xmlChar *key;
	cur = cur->xmlChildrenNode;

	struct guest_kernel_list *this_elt = malloc(sizeof(struct guest_kernel_list));
	struct guest_kernel *this_kernel = malloc(sizeof(struct guest_kernel));
	this_elt->kernel = this_kernel;
	this_elt->next = NULL;

	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"phys_cpu"))) {
			printf("phys_cpu: %s\n", key);
			this_kernel->phys_cpu = atoi(key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *)"mem_start"))) {
			printf("mem_start: %s\n", key);
			this_kernel->mem_start = strtoul(key, NULL, 0);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *)"mem_size"))) {
			printf("mem_size: %s\n", key);
			this_kernel->mem_size = strtoul(key, NULL, 0);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *)"kernel_addr"))) {
			printf("kernel_addr: %s\n", key);
			this_kernel->kernel_addr = strtoul(key, NULL, 0);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *)"ramdisk_addr"))) {
			printf("ramdisk_addr: %s\n", key);
			this_kernel->ramdisk_addr = strtoul(key, NULL, 0);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *)"pci_flags"))) {
			printf("pci_flags: %s\n", key);
			strcpy(this_kernel->pci_flags, key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *)"console"))) {
			printf("console: %s\n", key);
			strcpy(this_kernel->console, key);
		}

		cur = cur->next;
	}

	/* If we didn't encounter any errors, put this node in the list */

	if (!list_head) {
		list_head = this_elt;
		list_elt = this_elt;
	} else {
		list_elt->next = this_elt;
		list_elt = this_elt;
	}

	return;
}

static void
parseDoc(char *docname) {

	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname);

	if (doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		return;
	}

	cur = xmlDocGetRootElement(doc);

	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "mklinux_params")) {
		fprintf(stderr,"document of the wrong type, root node != mklinux_params");
		xmlFreeDoc(doc);
		return;
	}
	
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		//printf("item name: %s\n", (char *)cur->name);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"overall"))) {
			parseOverall(doc, cur);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *)"kernel"))) {
			parseKernel(doc, cur);
		}

		cur = cur->next;
	}
	
	xmlFreeDoc(doc);
	return;
}

int copyRamdisk(char *filename, unsigned long ramdisk_addr) {
	int mem_fd;
	uint64_t ramdisk_size, size_read;
	void *ramdisk_base_addr;
	FILE *file = fopen(filename, "rb");

	printf("Copying ramdisk...\n");

	if (!file) {
		printf("Failed to open file containing ramdisk...\n");
		return -1;
	}

	/* Open the file with the ramdisk and determine its size */
	fseek(file, 0, SEEK_END);
	ramdisk_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	printf("Ramdisk size is %lu bytes\n", ramdisk_size);

#ifndef DEBUG

	/* Open /dev/mem and mmap the boot arguments */
	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (mem_fd < 0) {
		printf("Failed to open /dev/mem!\n");
		return 0;
	}

	//printf("Opened /dev/mem, fd %d\n", mem_fd);
	ramdisk_base_addr = mmap(0, ramdisk_size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, ramdisk_addr);
	printf("Ramdisk at 0x%lx, mapped addr 0x%lx\n", ramdisk_addr, ramdisk_base_addr);

	/* Read the ramdisk into memory */
	size_read = fread(ramdisk_base_addr, 1, ramdisk_size, file);

	if (size_read != ramdisk_size) {
		printf("Failed to read the entire ramdisk into memory!\n");
		return 0;
	}

	munmap(ramdisk_base_addr, ramdisk_size);

	close(mem_fd);
#endif

	return;
}

void loadKernel(unsigned long kernel_addr) {
	int rc;
	char cmd[256];

	printf("Loading kernel...\n");

	sprintf(cmd, "kexec -d -a 0x%lx -l /kernel/vmlinux.elf -t elf-x86_64 --args-none", kernel_addr);

	rc = system(cmd);

	if (rc) {
		printf("Error %d returned when calling kexec!\n", rc);
	}

	return;
}

void bootKernel(struct guest_kernel *kernel) {
	printf("Booting physical CPU %d\n", kernel->phys_cpu);

	/* Copy the ramdisk */
	copyRamdisk(my_common_params.ramdisk, kernel->ramdisk_addr);

	/* Set the boot arguments */
	//setBootArgs(kernel->console, kernel->mem_start, kernel->mem_size, kernel->pci_flags, kernel->phys_cpu);

	/* Load the kernel */
	loadKernel(kernel->kernel_addr);

	/* Boot the kernel */

	return;
}


int main(int argc, char **argv) {
	char *docname;
	struct guest_kernel_list *it;

	if (argc <= 1) {
		printf("Usage: %s docname\n", argv[0]);
		return(0);
	}

	docname = argv[1];

	/* First, parse the document */
	parseDoc (docname);

	/* Next, boot the kernels. */
	it = list_head;
	while (it) {
		bootKernel(it->kernel);
		it = it->next;
	}

	return (1);

}
