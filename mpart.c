// most from
// numactl-2.0.8-rc5.tar.gz/libnuma.c

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>



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

/*
 * get the total (configured) number of cpus - both online and offline
 */

static int
numa_configured_cpus(void)
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

int maxconfigurednode = 0;

static void
set_configured_nodes(void)
{
	DIR *d;
	struct dirent *de;
	long long freep;

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
}




long long maxpresentmem = 0;

typedef struct _memres {
  long long start, end;
} memres;

#define MAX_AMEMRES 64
memres amemres[MAX_AMEMRES];

static int
set_present_mem(void)
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
			
		total += (end -start);
		amemres[im].start = start;
		amemres[im].end = end;
		im++;
		printf("start %llx end %llx amount %lld B 0x%llx\n", start, end, (end -start), (end -start) +1);
	} 
	fclose(f); 
	free(line);
	printf("total %lld %lld kB %lld MB %lld GB\n", total, total >>10, total >>20, total >>30);
	
	maxpresentmem = total;	
	return 0;
}

typedef struct _numa_node{
  long long start, end; //physical start and end
  long long size; // numa memory size
  memres * rstart, * rend; //pointer to start, end resource area
} numa_node;


//allocation is done in reverse order (hopefully is correct per node)


int main(int argc, char* argv[])
{ 
  set_configured_nodes();
  
/*  printf("possible nodes %d cpus %d\n",
	numa_num_possible_nodes(),
 	numa_num_possible_cpus()
	);
  */
  printf("configured nodes %d cpus %d\n",
//	 numa_num_configured_nodes(),
	 	 (maxconfigurednode +1),
//	 numa_num_configured_cpus()
	 (numa_configured_cpus() +1)
	);
  
  long long total = 0, size = 0;
  int i;
  for (i=0; i< (maxconfigurednode +1); i++) {
     size= numa_node_size64(i, 0);
     total += size;
    printf("node %d mem %lld %llx\n",
	   i, size, size);
  }
    
  printf ("total = %lld %lld kB %lld MB %lld GB\n\n", total, total >>10, total >>20, total >>30);
  
  /////////////////////////////////////////////////////////////////////////////
    
  set_present_mem();
  
  int im= -1;
  for (i=0; i<MAX_AMEMRES; i++)
    if ((amemres[i].start == 0) && (amemres[i].end == 0))
      break;
    im = i -1;
  
  unsigned long long last = amemres[im].end;
  unsigned long long nlast;
  printf("total memory hole %llu B %llu kB\n\n", (last - total), (last - total) >> 10 );
  
  //allocate descriptors
  numa_node * anode = malloc(sizeof(numa_node) * (maxconfigurednode +1));
  if (!anode) {
    printf("malloc returned zero exit\n");
    return -1;
  }
  memset(anode, 0, (sizeof(numa_node) * (maxconfigurednode +1)));
  
  for (i=maxconfigurednode; i> -1 ; i--) {
     size= numa_node_size64(i, 0);
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
     anode[i].rstart = &(amemres[im]);;
     
    printf("node %d mem %lld %llx - start %llx end %llx (size %llu)\n",
	   i, anode[i].size, anode[i].size, anode[i].start, anode[i].end, ( anode[i].end - anode[i].start));
    last = nlast;    
  }

// the following is outputted from linu/arch/x86/mm/numa.c in  setup_node_data
  printf("\n\nCROSSCHECK for consistency with\n"
	 "dmesg | grep NODE_DATA\n"
	 "dmesg | grep \"Initmem setup node\"\n");
  
  return 0;
}


