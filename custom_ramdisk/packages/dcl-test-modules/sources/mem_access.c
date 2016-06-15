/* Virtual address access test
 * ===========================
 * Usage: 
 * # insmod module.ko addr=<virtual address to access>
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <asm/page.h>

#define PRINT_PREF "[MemAccess] "

ulong addr = 0x0000000000000000;
module_param(addr, ulong, 0);

static int __init mymodule_init(void)
{
	char *ptr = (char *)addr;
	
	if(addr == 0x0000000000000000)
	{
		printk(PRINT_PREF "Please indicate virtual address to access "
			"using the addr=<address> parameter\n");
		return -1;
	}
	
	printk(PRINT_PREF "Into module.\n");
	printk(PRINT_PREF "Pointed character: %c\n", *ptr);

	return 0;
}

static void __exit mymodule_exit(void)
{
 printk ("exit module.\n");
        return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
