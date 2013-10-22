
typedef struct _memres {
  long long start, end;
} memres;

#define BASE_MEMRES 16

long long numa_node_size64(int node, long long *freep);

int numa_node_cpumask(int node, cpu_bitmask_t * cpus);




int numa_configured_cpus(void);

int set_configured_nodes(void);

