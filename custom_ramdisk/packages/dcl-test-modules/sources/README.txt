This is a set of test modules to check that kernel non-overlapping 
virtual address space is working. They are suppopsed to work when 
defining the two replicas address spaces according to the rules
described in arch/x86/include/asm/pierre_dcl.h.

To compile the modules edit the KDIR variable in the Makefile to point 
to the root of the kernel sources.

1) Direct memory mapping
------------------------
Here we try to access the direct mapped memory area and show that what 
is valid in one replica is crashing the other one. Note that with 
Qemu when we crash the kernel it is better to completely shutdown Qemu 
and restart it. I noticed that doing only 'reboot' might sometimes lead 
to a crash in the boot process ...

Use the dmem_access module to directly access (read) a virtual address,
usage is:
# insmod insmod mem_access.ko addr=<address to read>

Working on replica1 and crashing on replica2:
insmod mem_access.ko addr=0xffff880000000000 && rmmod mem_access.ko

Working on replica2 and crashing on replica1:
insmod mem_access.ko addr=0xffff980000000000 && rmmod mem_access.ko

AFAIK, everything from 0xffff880000000000 to 0xffff880000000000 + 
your amount of RAM should work in replica1 and crash in replica2. On my 
test platform with Qemu and 4G RAM, access to @ like ffff8800dff11000 
crashes on replica1, although this is supposed to land in the 4G area
... Why I do not know, maybe the entire amount of physical memory is not
mapped (for example some reserved space is not mapped ?)

2) Vmalloc space
----------------
For verifying the non overlapping vmalloc spaces I wrote a module 
allocating n chunks of m MB and checking that each chunk falls in the 
right virtual area. I was able to successfully check on each replica 
3800 MB / 4GB of total physical memory. If I try more than 3800 MB I'm 
getting what looks like OOM errors (which makes sense).

The module is called vmalloc2.ko and its usage is:
# insmod vmalloc2.ko num=<numvber of chunks to allocate> size=<size
	of one chunk in MB>

This works on my 4GB virtual machine:
# insmod vmalloc2.ko num=38 size=100

Note that this module should be compiled against the correct replica.

ALSO: I checked the implementation of vmalloc (probably something I 
should have done before writing the module). It is in mm/vmalloc.c. The
vmalloc path goes through __vmalloc_node which itself calls
__vmalloc_node_range with several parameters and in particular the
hardcoded VMALLOC_START and VMALLOC_END (which are the value we changed
when defining the replica virtual address space). __vmalloc_node_range
triggers a chain of functions that allocate memory in the specified 
range, i.e. between VMALLOC_START and VMALLOC_END so we should be good.

Using mem_access, we can try to see that accessing in each replica
outside of the struct page array area is valid in one and crashes the
second:

Working on replica1 and crashing on replica2:
insmod mem_access.ko addr=0xffffc90000000000 && rmmod mem_access.ko

Working on replica2 and crashing on replica1:
insmod mem_access.ko addr=0xffffdc8000000000 && rmmod mem_access.ko

3) struct page array (virtual memory map)
-----------------------------------------
The module page_array.ko gets a pointer on the struct page 
corresponding to PFN 0, 1, and access one arbitrary field (flags) as 
a way to dereference the pointer. It also print the address of the
pointer so it can be used to check it is located in the right area for
both replicas.

The module also tries to get a pointer on the struct page for PFN 
786431. This is actually hardcoded in the module and it corresponds to
the last struct page of the array we can access on my Qemu with 4GB of
RAM. Changing machine and one might need to modify this value. To find
the last available struct page there is a simple loop commented in the
module that access sequentially every struct page. Just take the last
one working before the crash :)

As the first and last struct page are located in the right VM area for
both replicas, and we know that they are all contiguous in VM, we are
good.

Using mem_access, we can try to see that accessing in each replica
outside of the struct page array area is valid in one and crashes the
second:

Working on replica1 and crashing on replica2:
insmod mem_access.ko addr=0xffffea0000000000 && rmmod mem_access.ko

Working on replica2 and crashing on replica1:
insmod mem_access.ko addr=0xffffea8000000000 && rmmod mem_access.ko

4) Module area
--------------
The module named module_area.ko uses the __builtin_return_address 
function to get a pointer on the return address of the function in which
__builtin_return_address is called. So we have to call a function f from
the init function of the module, and then call __builtin_return_address 
from f. Thus we can check that the address is correctly located in the
right area.

Using mem_access, we can try to see that accessing in each replica
outside of the struct page array area is valid in one and crashes the
second:

Working on replica1 and crashing on replica2:
insmod mem_access.ko addr=0xffffffffa0000000 && rmmod mem_access.ko

Working on replica2 and crashing on replica1:
insmod mem_access.ko addr=0xffffffffcf000000 && rmmod mem_access.ko

5) Kernel executable
--------------------
It is easy to check that the virtual addresses where the kernel is 
mapped are correct with objdump on the kernel executable:

objdump -x vmlinux | more

We should see virtual addresses like 0xffffffff8XXXXXXX on replica1 and
0xffffffff9XXXXXXX on replica2.

Using mem_access, we can try to see that accessing in each replica
outside of the struct page array area is valid in one and crashes the
second:

Working on replica1 and crashing on replica2:
insmod mem_access.ko addr=0xffffffff81000000 && rmmod mem_access.ko

Working on replica2 and crashing on replica1:
insmod mem_access.ko addr=0xffffffff91000000 && rmmod mem_access.ko
