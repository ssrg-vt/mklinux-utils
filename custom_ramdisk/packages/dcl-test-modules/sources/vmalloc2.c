/* Vmalloc allocation test
 * =======================
 * Allocate 'num' chunks of 'size' MB in vmalloc area
 * Usage: 
 * # insmod module.ko num=<number of chunks to allocate> size=<size of a 
 * 		chunk to allocate in MB>
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <asm/pierre_dcl.h>

#define PRINT_PREF "[Vmalloc2] "

/* switch on replica num. */
#ifdef DCL_REP1
int replica_num=1;
#elif defined DCL_REP2
int replica_num=2;
#else
#error "Not compiled against replica kernel"
#endif /* replica switch */

int size = 0;
module_param(size, int, 0);

int num = 0;
module_param(num, int, 0);

int check_ptr(ulong ptr, ulong lower_bound, ulong upper_bound);

static int __init mymodule_init(void)
{
	void *ptr, **pointers;
	int size_mb, i, success;
	ulong lower_bound, upper_bound;
	
	size_mb = size;
	if(size_mb == 0)
	{
		printk(PRINT_PREF "Usage: insmod module.ko num=<number of "
			"chunks to allocate> size=<size of a chunk to allocate in "
			"MB>\n");
		return -1;
	}
	
	lower_bound = upper_bound = 0;
	if(replica_num == 1)
	{
		lower_bound = DCL_VMALLOC_START_REP1;
		upper_bound = DCL_VMALLOC_END_REP1;
	}
	else /* replica_num == 2 */
	{
		lower_bound = DCL_VMALLOC_START_REP2;
		upper_bound = DCL_VMALLOC_END_REP2;
	}
	
	/* allocate an array to store the pointers */
	pointers = (void **)vmalloc(num * sizeof(void *));
	
	/* allocate the chunks */
	for(i=0; i<num; i++)
		pointers[i] = (void *)vmalloc(size_mb * 1024 * 1024);
	
	//~ /* Verify everyone */
	success =1;
	for(i=0; i<num; i++)
	{
		ulong last_allocated_byte = 0;
		ptr = pointers[i];
		last_allocated_byte = (ulong)ptr+(size_mb*1024*1024);
		
		/* check that the pointer falls into the correct VM range */
		if (!check_ptr((ulong)ptr, lower_bound, upper_bound) || 
			!check_ptr(last_allocated_byte, lower_bound, upper_bound))
		{
			printk(PRINT_PREF "ERROR checking chunk @%p + %x against "
				"range %lx - %lx\n", ptr, size_mb*1024*1024, lower_bound,
				upper_bound);
			success = 0;
			break;
		}
		
		printk(PRINT_PREF "Checked @%lx - %lx\n", (ulong)ptr, 
			last_allocated_byte);
		
	}
	
	if(success)
		printk(PRINT_PREF "Successfully verified %d chunks of %d MB.\n",
		num, size_mb);
	
	/* free the chunks */
	for(i=0; i<num; i++)
		vfree(pointers[i]);
	
	vfree(pointers);
	return 0;
}

/* Check that ptr is located between upper and lower bound (included) */
int check_ptr(ulong ptr, ulong lower_bound, ulong upper_bound)
{
	return (ptr >= lower_bound) &&
		(ptr <= upper_bound);
}

static void __exit mymodule_exit(void)
{
 printk ("exit module.\n");
        return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
