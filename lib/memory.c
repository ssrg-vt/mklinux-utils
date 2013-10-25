

// todo but must return a linked list of system ram with the last element pointing to 0 0xFFFFFFFFF if error otr ZERO?!


// EXTERNAL CREDITS
// most parsing function from
// numactl-2.0.8-rc5.tar.gz/libnuma.c

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

#include "memory.h"
#include "bits.h"



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

///////////////////////////////////////////////////////////////////////////////
// The following requires NUMA support compiled in the kernel

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
	return maxconfigurednode;
}

struct numa * numares_init(void)
{
	long long total = 0, size = 0, cmap = 0;
	int i;

	maxconfigurednode = TODO_HERE;

	for (i=0; i< (maxconfigurednode +1); i++) {
		size = numa_node_size64(i, 0);
		cpu_bitmask_t a;
		numa_node_cpumask(i, &a);
		total += size;
		printf("node %d mem %lld %llx map TODOllx (%d core)\n",
				i, size, size, bit_weight_bitmask(&a));
	}

	printf ("total = %lld %lld kB %lld MB %lld GB\n\n", total, total >>10, total >>20, total >>30);
	total_by_node = total; //unsigned long long total_by_node=-1;
}

numa_memres_combine
{
for (i=maxconfigurednode; i> -1 ; i--) {
    size= numa_node_size64(i, 0);
    numa_node_cpumask(i, &(anode[i].map));
         //print_bitmask(&(anode[i].map)); printf(" (%d) \n", anode[i].cpus);
    anode[i].cpus = bit_weight_bitmask(&(anode[i].map)); //number of cpus
 //print_bitmask(&(anode[i].map)); printf(" (%d) \n", anode[i].cpus);

    anode[i].size = size;
    anode[i].end = last;
    anode[i].rend = &(amemres[im]);

    while ((last - amemres[im].start) < size) {
      if ( !(im >0) ) {
	 printf("THERE IS NO MORE MEMORY\n");
	 break;

      }

     unsigned long long avail = last - amemres[im].start; //available space
     printf("hole is present on these node - PREV start %llx last %llx avail %llu (%llu kB) CURR end %llx start %llx\n",
	     amemres[im].start, last, avail, (avail >> 10), amemres[(im -1)].end, amemres[(im -1)].start);
     im--;
     size -= avail;
     last = amemres[im].end;
    }

    nlast = last - size;
    anode[i].start = nlast;

    anode[i].rstart = &(amemres[im]);

   printf("node %d mem %lld %llx - start %llx (%lldMB) end %llx (size %llu)\n",
	   i, anode[i].size, anode[i].size, anode[i].start, anode[i].start >> 20, anode[i].end, ( anode[i].end - anode[i].start));
   last = nlast;
 }
}


memres_node_from_dmesg() {

	create a tmp file

    //int a = system ("cacca &> /dev/null");
    int a = system ("cacca");
//      int b = system ("dmesg");
    int c = system ("dmesg | grep \"Initmem setup node\" > tmpfile_c.log");
    int d = system ("dmesg | grep \"grande cacca\" > tmpfile_d.log");

    if (c == 0) continue everything is ok

    open the tmp file

    parse it

    remove the tmp file

//      printf(" excuted a %d b %d c %d\n", a, b, c);
     printf(" excuted a %d c %d d %d\n", a, c, d);
RETURNS:
sh: cacca: not found (because for sure cacca is not there)
	excuted a 32512 c 0 d 256

}


///////////////////////////////////////////////////////////////////////////////
// the following are generic functions (must be supported on any kernel)

/*
 * get the total (configured) number of cpus - both online and offline
 */

//number of cpus can also be retrieved from /proc/cpuinfo
//or /sys/devices/system/cpus/cpuXYZ
// move to another file?!
int configured_cpus(void)
{
	int maxconfiguredcpu = sysconf(_SC_NPROCESSORS_CONF) - 1;
	if (maxconfiguredcpu == -1)
		printf("sysconf(NPROCESSORS_CONF) failed.\n");
	return maxconfiguredcpu;
}

// this is system_ram
// returns zero if error
// can return error if the user does not have enough privileges to open /proc/iomem ?!
// right now it returns an array, we can implement a single linked list eventually
// because I love libc I used the following convention: index -1 gives the number of elements in the list
struct memres * memres_init (void)
{
	int ok = 0;
	size_t len = 0;
	char *line = NULL;

	long long total = 0;
	long long end = -1;
	long long start = -1;
	FILE *f;

	int memres_size = BASE_MEMRES;
	struct memres * memlist = malloc(memres_size +1);
	if (!memlist) {
		printf ("malloc failed");
		return NULL;
	}
	memlist[0].count = memres_size;
	memlist[0].elements = 0;
	memlist = memlist +1;
	memset(memlist, 0, sizeof(memres) * memres_size);

	int im = 0;

	f = fopen("/proc/iomem", "r");
	if (!f) {
		printf("failed to open /proc/iomem");
		return NULL;
	}

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

		memlist[im].start = start;
		memlist[im].end = end;
		memlist[-1].elements ++;
		im++;
		if (!(im < memres_size)) {
			memres_size *= 2;
			struct memres * _memlist = malloc(memres_size +1);
			if (!_memlist) {
				printf ("malloc failed in resizing the ar  ray");
				free((memlist -1));
				return NULL;
			}
			_memlist = _memlist +1;
			_memlist[-1].count = memres_size;
			_memlist[-1].elements = memlist[-1].elements;
			memset(_memlist, 0, sizeof(memres) * memres_size);

			for (i=0; i<_memlist[-1].elements; i++)
				_memlist[i] = memlist[i];

			free((memlist -1));
			memlist = _memlist;
		}

		// if debug print the following line
		printf("start %llx end %llx amount %lld B 0x%llx\n", start, end, (end -start), (end -start));
	}
	fclose(f);
	free(line);

	return memlist;
}

void memres_free (struct memres * memlist)
{
	free((memlist -1));
}

long long mem_total_available () {
	printf("total %lld %lld kB %lld MB %lld GB\n", total, total >>10, total >>20, total >>30);
}


