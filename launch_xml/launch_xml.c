/* this is launch_xml.c, a parser to perform MKLinux launch */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "launch_xml.h"

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

	if (!list_head) {
		list_head = malloc(sizeof(struct guest_kernel_list));
		list_head->next = NULL;
		list_elt = list_head;
	}

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
		}

		cur = cur->next;
	}

	/* If we didn't encounter any errors, put this node in the list */

	if (!list_head) {
		list_head = this_elt;
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


int main(int argc, char **argv) {
	char *docname;
			
	if (argc <= 1) {
		printf("Usage: %s docname\n", argv[0]);
		return(0);
	}

	docname = argv[1];
	parseDoc (docname);

	return (1);

}
