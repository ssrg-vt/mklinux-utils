#ifndef XML_LAUNCH_H
#define XML_LAUNCH_H

struct common_params {
	unsigned long lapic_timer;
};

struct guest_kernel {
	unsigned int phys_cpu;
	unsigned long kernel_addr;
	unsigned long ramdisk_addr;
	unsigned long mem_start;
	unsigned long mem_size;
	char pci_flags[32];

};

struct guest_kernel_list {
	struct guest_kernel *kernel;
	struct guest_kernel_list *next;
};

#endif /* XML_LAUNCH_H */
