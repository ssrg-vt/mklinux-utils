
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <stdio.h>
#include <sched.h>

/* NOTE for dynamically sized CPU sets there are the following
CPU_ALLOC
CPU_ALLOC_SIZE
CPU_FREE

for the above MACROs we should use the following functions
CPU_ZERO_S
CPU_SET_S
CPU_CLR_S
CPU_ISSET_S
CPU_COUNT_S
...
*/

int main(int argc, char* argv[])
{
    int ret, i;
    char * pcpu;
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    printf("sizeof cpu_set_t is %ld (bytes) %d (elems)\n", sizeof(cpu_set_t), CPU_SETSIZE);

    ret = sched_getaffinity(0, sizeof(cpu_set_t), &cpu_mask);
    if (ret) {
	perror("sched_getaffinity");
	return ret;
    }
    printf("CPU_COUNT %d\n", CPU_COUNT(&cpu_mask));
    pcpu = (char*) &cpu_mask;
    for (i=0; i< sizeof(cpu_set_t); i++) {
        printf("%.02x", ((unsigned int)pcpu[i] & 0xFF));
        if (!((i+1) % 4))
            printf(",");
    }
    printf("\n");

    return 0;
}

