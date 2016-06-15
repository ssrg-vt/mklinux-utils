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

#define PRINT_PREF "[ModuleArea] "

void a_function(void);
void a_second_function(void);

static int __init mymodule_init(void)
{
	void *ptr;
	
	ptr = __builtin_return_address(0);
	
	printk(PRINT_PREF "(main) __builtin_return_address(0) == %p\n", ptr);
	
	a_function();
	
	return 0;
}

void a_function(void)
{
	void *ptr;
	ptr = __builtin_return_address(0);
	printk(PRINT_PREF "(func) __builtin_return_address(0) == %p\n", ptr);
	
	a_second_function();
}

void a_second_function(void)
{
	void *ptr;
	ptr = __builtin_return_address(0);
	printk(PRINT_PREF "(2ndfunc) __builtin_return_address(0) == %p\n", ptr);
}

static void __exit mymodule_exit(void)
{
 printk ("exit module.\n");
        return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
