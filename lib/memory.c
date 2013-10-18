

// todo but must return a linked list of system ram with the last element pointing to 0 0xFFFFFFFFF if error otr ZERO?!


// EXTERNAL CREDITS
// most parsing function from
// numactl-2.0.8-rc5.tar.gz/libnuma.c

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>



typedef struct _numa_node{
  long long start, end; //physical start and end
  long long size; // numa memory size
  memres * rstart, * rend; //pointer to start, end resource area
  cpu_bitmask_t map; // map of the cpus
  int cpus;
} numa_node;
//allocation is done in reverse order (hopefully is correct per node)


#define MAX_BITMAP_CPUS 8
typedef struct _cpu_bitmask {
  int cpus[MAX_BITMAP_CPUS];
} cpu_bitmask_t;


unsigned long long total_by_node=-1;



int system_ram( /*return a list */ )
{
  int ok = 0;
  size_t len = 0;
  char *line = NULL;
  long long total = 0;
  long long end = -1;
  long long start = -1;
  FILE *f;
  int im = 0;

  memset(amemres, 0, sizeof(memres) * MAX_AMEMRES);

  f = fopen("/proc/iomem", "r");
  if (!f)
    return -1;
  while (getdelim(&line, &len, '\n', f) > 0) {
    char *endptr;
    char *s = strcasestr(line, "System RAM");
    if (!s)
      continue;
    --s;
    while (s > line && isspace(*s)) // space
      --s;
    while (s > line && ispunct(*s)) // column
      --s;
    while (s > line && isspace(*s)) // space
      --s;
    while (s > line && isxdigit(*s)) // hex end address
      --s;

    end = strtoull((s +1),&endptr,16);
    if (endptr == s)
      end = -1;
    else
      ok++;

    while (s > line && ispunct(*s)) // column
      --s;
    while (s > line && isxdigit(*s)) // hex end address
      --s;

    start = strtoull((s > line) ? (s +1) : s,&endptr,16);
    if (endptr == s)
      end = -1;
    else
      ok++;

    end++;
    total += (end -start);

#define ALIGN4KB
#ifdef ALIGN4KB
		start &= ~0x0FFF;
		end &= ~0x0FFF;
#endif

    //add to list
		amemres[im].start = start;
		amemres[im].end = end;
		im++;
		printf("start %llx end %llx amount %lld B 0x%llx\n", start, end, (end -start), (end -start));
	}
  fclose(f);
  free(line);
  printf("total %lld %lld kB %lld MB %lld GB\n", total, total >>10, total >>20, total >>30);

  maxpresentmem = total;
  return 0;
}

/* (cache the result?) */
long long numa_node_size64(int node, long long *freep)
{
	size_t len = 0;
	char *line = NULL;
	long long size = -1;
	FILE *f;
	char fn[64];
	int ok = 0;
	int required = freep ? 2 : 1;

	if (freep)
		*freep = -1;
	sprintf(fn,"/sys/devices/system/node/node%d/meminfo", node);
	f = fopen(fn, "r");
	if (!f)
		return -1;
	while (getdelim(&line, &len, '\n', f) > 0) {
		char *end;
		char *s = strcasestr(line, "kB");
		if (!s)
			continue;
		--s;
		while (s > line && isspace(*s))
			--s;
		while (s > line && isdigit(*s))
			--s;
		if (strstr(line, "MemTotal")) {
			size = strtoull(s,&end,0) << 10;
			if (end == s)
				size = -1;
			else
				ok++;
		}
		if (freep && strstr(line, "MemFree")) {
			*freep = strtoull(s,&end,0) << 10;
			if (end == s)
				*freep = -1;
			else
				ok++;
		}
	}
	fclose(f);
	free(line);
	if (ok != required)
		printf("Cannot parse sysfs meminfo (%d)", ok);
	return size;
}


//ideas from linux/lib/bitmap.c __bitmap_parse (chunk oriented)
int numa_node_cpumask(int node, cpu_bitmask_t * cpus)
{
	size_t len = 0;
	char *line = NULL;
	//unsigned long map[MAX_BITMAP_CPUS];
	memset(cpus, 0, sizeof(cpu_bitmask_t));
	char *pmap = (char*) cpus;
//	pmap += (sizeof(cpu_bitmask_t) -1);

	FILE *f;
	char fn[64];
	int i;
	int digit=0;

	sprintf(fn,"/sys/devices/system/node/node%d/cpumap", node);
	f = fopen(fn, "r");
	if (!f) {
	  printf("ERROR ERROR\n");
	 	return -1;
	}
	while (getdelim(&line, &len, '\n', f) > 0) {

	  //printf("%d:%s*%d   %p  %d\n", len, line, strlen(line), line, sizeof(long long));
	  int llen = strlen(line);

	  for (i=0; i<llen ; i++) {

	    if (isspace(line[llen -i]))
	      continue;
	    if (line[llen -i] == ',')
	      continue;
	    if (line[llen -i] == '\n' || line[llen -i] == '\0')
	      continue;
	    if (!isxdigit(line[llen -i])) {
	      printf("numa_node_cpumask error '%c' %d digit %d\n", line[llen -i], (int)line[llen -i], digit);
	      return -1;
	    }

	    if (!(digit & 0x1)) {
	      *pmap = (hex_to_bin(line[llen -i]) & 0xF) ;
	      //pmap++
	    }else {
	      *pmap |= (hex_to_bin(line[llen -i]) & 0xF ) << 4;
	      pmap++;
	    }

	    digit++;

	    if (pmap > ((void*)cpus + sizeof(cpu_bitmask_t)) || pmap < ((void*)cpus))
	      goto __end;
	  }
	}
__end:
	fclose(f);
	free(line);
	//printf("digit %d\n",digit);

	return digit;
}



/*
 * get the total (configured) number of cpus - both online and offline
 */

int numa_configured_cpus(void)
{
	int maxconfiguredcpu = sysconf(_SC_NPROCESSORS_CONF) - 1;
	if (maxconfiguredcpu == -1)
		printf("sysconf(NPROCESSORS_CONF) failed.\n");
	return maxconfiguredcpu;
}

/*
 * Find nodes (numa_nodes_ptr), nodes with memory (numa_memnode_ptr)
 * and the highest numbered existing node (maxconfigurednode).
 */

int set_configured_nodes(void)
{
	DIR *d;
	struct dirent *de;
	long long freep;
	int maxconfigurednode = 0;

//	numa_memnode_ptr = numa_allocate_nodemask();
//	numa_nodes_ptr = numa_allocate_nodemask();

	d = opendir("/sys/devices/system/node");
	if (!d) {
		maxconfigurednode = 0;
	} else {
		while ((de = readdir(d)) != NULL) {
			int nd;
			if (strncmp(de->d_name, "node", 4))
				continue;
			nd = strtoul(de->d_name+4, NULL, 0);
//			numa_bitmask_setbit(numa_nodes_ptr, nd);
//			if (numa_node_size64(nd, &freep) > 0)
//				numa_bitmask_setbit(numa_memnode_ptr, nd);
			if (maxconfigurednode < nd)
				maxconfigurednode = nd;
		}
		closedir(d);
	}
	return maxconfigurednode
}



// this is system_ram
static int set_present_mem(void)
{
  int ok = 0;
	size_t len = 0;
	char *line = NULL;

	long long total = 0;
	long long end = -1;
	long long start = -1;
	FILE *f;

	int im = 0;
	memset(amemres, 0, sizeof(memres) * MAX_AMEMRES);

	f = fopen("/proc/iomem", "r");
	if (!f)
		return -1;
	while (getdelim(&line, &len, '\n', f) > 0) {
		char *endptr;
		char *s = strcasestr(line, "System RAM");
		if (!s)
			continue;
		--s;
		while (s > line && isspace(*s)) // space
			--s;
		while (s > line && ispunct(*s)) // column
			--s;
		while (s > line && isspace(*s)) // space
			--s;

		while (s > line && isxdigit(*s)) // hex end address
			--s;

		end = strtoull((s +1),&endptr,16);
		if (endptr == s)
				end = -1;
			else
				ok++;

		while (s > line && ispunct(*s)) // column
			--s;

		while (s > line && isxdigit(*s)) // hex end address
			--s;

		start = strtoull((s > line) ? (s +1) : s,&endptr,16);
		if (endptr == s)
				end = -1;
			else
				ok++;

		end++;
		total += (end -start);

#define ALIGN4KB
#ifdef ALIGN4KB
		start &= ~0x0FFF;
		end &= ~0x0FFF;
#endif /* !ALIGN4KB */

		amemres[im].start = start;
		amemres[im].end = end;
		im++;
		printf("start %llx end %llx amount %lld B 0x%llx\n", start, end, (end -start), (end -start));
	}
	fclose(f);
	free(line);
	printf("total %lld %lld kB %lld MB %lld GB\n", total, total >>10, total >>20, total >>30);

	maxpresentmem = total;
	return 0;
}


