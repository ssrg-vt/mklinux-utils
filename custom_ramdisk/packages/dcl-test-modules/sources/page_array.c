/* Struct page array location test
 * ===============================
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <asm/page.h>
#include <linux/mm_types.h>
#include <asm/pierre_dcl.h>

#define PRINT_PREF "[PageArray] "

/* This can be found by running the loop in the end of the module code
 * and taking the last entry before the crash. Maybe there are more
 * intelligent ways to find it ... */
#define LAST_ACCESSIBLE_PAGE_FRAME		786431

/* switch on replica num. */
#ifdef DCL_REP1
int replica_num=1;
#elif defined DCL_REP2
int replica_num=2;
#else
#error "Not compiled against replica kernel"
#endif /* replica switch */

static int __init mymodule_init(void)
{
	struct page *p;
	ulong i; (void)i;
	
	printk(PRINT_PREF "sizeof(struct page) is 0x%lx\n", sizeof(struct page));
	
	p = pfn_to_page(0);
	printk(PRINT_PREF "struct page for PFN 0 is located @%p, flags: %lu\n",
	 p, p-> flags);
	
	p = pfn_to_page(1);
	printk(PRINT_PREF "struct page for PFN 1 is located @%p, flags: %lu\n",
	 p, p-> flags);
	 
	/* For some reason I cannot go above 786431 pages (which is around 
	 * 3072 MB but I have 4 GB of RAM)
	 */
	p = pfn_to_page(LAST_ACCESSIBLE_PAGE_FRAME);
	printk(PRINT_PREF "struct page for PFN %d is located @%p, "
		"flags: %lu\n", LAST_ACCESSIBLE_PAGE_FRAME, p, p-> flags);
	 
	/* Scan all the memory frames */
	//~ for(i=0; i<(((1024*1024*1024)/4096)*4); i++)
	//~ {
		//~ p = pfn_to_page(i);
		//~ printk(PRINT_PREF "PFN %lu flags: %lu\n", i, p->flags);
	//~ }
	
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
