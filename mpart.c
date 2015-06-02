/*
 * Antonio Barbalace, SSRG, Virginia Tech 2012 - 2015
 */

/*
 * this code partition the memory to be used by mklinux
 * this is part of a set of scripts, prototype code.
 * Because of lack of time this code remains in prototype version.
 * Identation has been corrected with Eclipse
 */

// EXTERNAL CREDITS
// most parsing function from
// numactl-2.0.8-rc5.tar.gz/libnuma.c

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

//#include "bit.h"

unsigned long long total_by_node=-1;

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

int hex_to_bin(char ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return ch - '0';
	ch = tolower(ch);
	if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	return -1;
}

#define MAX_BITMAP_CPUS 8
typedef struct _cpu_bitmask {
	int cpus[MAX_BITMAP_CPUS];
} cpu_bitmask_t;
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

		end++;
		total += (end -start);

#define ALIGN4KB
#ifdef ALIGN4KB
		start &= ~0x0FFF;
		end &= ~0x0FFF;
#endif /* !ALIGN4KB */ 

		amemres[im].start = start; 
		amemres[im].end = end;
		printf("start %llx end %llx amount %lld B 0x%llx\n", start, end, (end -start), (end -start));

		im++;
		if (!(im < MAX_AMEMRES)) {
			printf("%s: ERROR: increase MAX_AMEMRES(%d) in the source code, not enough array entries.\n",
					__func__, MAX_AMEMRES);
			exit(1);
		}
	} 
	fclose(f); 
	free(line);
	printf("total %lld %lld kB %lld MB %lld GB\n", total, total >>10, total >>20, total >>30);

	maxpresentmem = total;	
	return 0;
}

long long maxpresentpci = 0;

memres apcires[MAX_AMEMRES];

static int
set_pci_holes(void)
{
	int ok = 0;
	size_t len = 0;
	char *line = NULL;

	long long total = 0;
	long long end = -1;
	long long start = -1;
	FILE *f;

	int im = 0;
	memset(apcires, 0, sizeof(memres) * MAX_AMEMRES);

	f = fopen("/proc/iomem", "r");
	if (!f)
		return -1;
	while (getdelim(&line, &len, '\n', f) > 0) {
		char *endptr;

		char *s = strcasestr(line, "PCI");
		if (!s) {
			//looking for alternative format before losing the hope
			//example: 0000:00:04.0 - really hand made
			s = strcasestr(line, "\n");
			while (s > line && isspace(*s)) // space
				--s;
			while (s > line && ispunct(*s)) // column
				--s;
			while (s > line && isspace(*s)) // space
				--s;

			if (!(s>line && isxdigit(*s)))
				continue;
			--s;
			if (!(s>line && (*s == '.')))
				continue;
			--s;
			if (!(s>line && isxdigit(*s)))
				continue;
			--s;
			if (!(s>line && isxdigit(*s)))
				continue;
			--s;
			if (!(s>line && (*s == ':')))
				continue;
			--s;
			if (!(s>line && isxdigit(*s)))
				continue;
			--s;
			if (!(s>line && isxdigit(*s)))
				continue;
			--s;
			if (!(s>line && (*s == ':')))
				continue;
			--s;
			if (!(s>line && isxdigit(*s)))
				continue;
			--s;
			if (!(s>line && isxdigit(*s)))
				continue;
			--s;
			if (!(s>line && isxdigit(*s)))
				continue;
			--s;
			if (!(s>line && isxdigit(*s)))
				continue;
			// here we are, in general I will print a warning if things are switching between different representations
			//TODO
		}

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
		if (s != line)
			continue; // because we only want root PCI areas

		end++;
		total += (end -start);

#define ALIGN4KB
#ifdef ALIGN4KB
		start &= ~0x0FFF;
		end &= ~0x0FFF;
#endif /* !ALIGN4KB */

		apcires[im].start = start;
		apcires[im].end = end;
		printf("PCI start %llx end %llx amount %lld B 0x%llx\n", start, end, (end -start), (end -start));

		im++;
		if (!(im < MAX_AMEMRES)) {
			printf("%s: ERROR: increase MAX_AMEMRES(%d) in the source code, not enough PCI array entries.\n",
					__func__, MAX_AMEMRES);
			exit(1);
		}
	}
	fclose(f);
	free(line);
	printf("PCI total %lld %lld kB %lld MB %lld GB\n", total, total >>10, total >>20, total >>30);

	maxpresentpci = total;
	return 0;
}

static int
check_intersections(memres *a, int asize, memres *b, int bsize) {
	int aa =0, bb =0;

	while (aa< asize && bb< bsize)
		if (a[aa].end <= b[bb].start)
		{ //printf("iomem mem %lx-%lx\n", a[aa].start, a[aa].end);
			aa++; }
		else
			if (b[bb].end <= a[aa].start)
			{ //printf("iomem pci %lx-%lx\n", b[bb].start, b[bb].end);
				bb++; }
			else {
				printf("%s: INTERSECT a[%d].start:%lx .end:%lx b[%d].start:%lx .end:%lx\n",
						__func__, aa, a[aa].start, a[aa].end, bb, b[bb].start, b[bb].end);
				exit(1); //return 0;
			}
	/*
	while (bb< bsize)
		{printf("iomem pci %lx-%lx\n", b[bb].start, b[bb].end); bb++; }
	while (aa< asize)
		{printf("iomem mem %lx-%lx\n", a[aa].start, a[aa].end); aa++; }
	 */

	return 1;
}

typedef struct _numa_node{
	long long start, end; //physical start and end
	long long size; // numa memory size
	memres * rstart, * rend; //pointer to start, end resource area
	cpu_bitmask_t map; // map of the cpus
	int cpus;
} numa_node;
//allocation is done in reverse order (hopefully is correct per node)

#include "lib/bits.c"

///////////////////////////////////////////////////////////////////////////////
// policies
///////////////////////////////////////////////////////////////////////////////

/* RESULTION is 2MB 0x200000 */
#define RESOLUTION_MASK ((1 << 21) -1)
int partitionedcpu_globalshm ( numa_node * list)
{
	//do the partition per node
	//long long reserved_cap = (0x10 << 20);   //use 16MB reservation at the beginning - it "was" an error
	long long reserved_cap = (0x1 << 20);   //use 1MB reservation at the beginning
	//  better idea just allocate from 0 to.. ?!?!
	// algorithm is: if over 16MB
	int i, l;

	// 512 MB
#define BEN_ALIGNMENT 0x20000000 
#ifdef BEN_ALIGNMENT
	printf ("Ben ALIGNMENT memmap=x@ALIGNED buond %lld\n", (long long) BEN_ALIGNMENT);
#endif
	printf("##### %s #####\n", __func__);

	for (i=0 ; i  < (maxconfigurednode +1) ; i++) {
		int cpu_num = list[i].cpus;
		long long size = list[i].size; //which size is it? total or avail?
		long long chunk = size / cpu_num;
		long long alignedchunk =  chunk & ~RESOLUTION_MASK;
		long long new_total = alignedchunk * cpu_num;
		long long diff = size -new_total;
		long long start = -1;
		if (cpu_num == 0) {
			printf("i %d cpu_num %d\n",i , cpu_num );
			return -1;
		}

		if (list[i].rstart == list[i].rend) {
			/// THIS NODE DOES NOT HAVE ANY HOLES IN ITS MAP -------------------------
			start =  list[i].start;
			cpu_bitmask_t mask = list[i].map;
			for (l=0; l<cpu_num; l++) {
				int ccpu = ffsll_bitmask(&mask) -1;

#ifdef BEN_ALIGNMENT
				if ( (start & (unsigned long long)~((unsigned long long)BEN_ALIGNMENT -1)) ) {
					start &= (unsigned long long)~((unsigned long long)BEN_ALIGNMENT -1);
					start += BEN_ALIGNMENT;
				}
#endif
				printf ("present_mask=%d memmap=%ldM@%ldM memmap=%ldM$%ldM mem=%ldM\n",
						ccpu,
						(unsigned long)alignedchunk >> 20, (unsigned long)start >> 20,
						(unsigned long)(start - reserved_cap) >> 20, (unsigned long)reserved_cap >> 20,
						(unsigned long)(start + alignedchunk) >> 20
				);
				start += alignedchunk;

				clearcpu_bitmask(&mask, ccpu);
			}
		}
		else {
			/// THIS NODE DOES HAVE HOLES SO MUST BE SPECIALLY HANDLED --------------
			cpu_bitmask_t mask = list[i].map;
			memres * smemres = list[i].rstart;
			start = smemres->start;

#define START_AFTER_CAP      
#ifdef START_AFTER_CAP
			//contains reserved_cap?
			while ( smemres->start < reserved_cap ) {
				if (smemres->end < reserved_cap) { // the hole is inside the cap
					smemres += 1;
					start = smemres->start;
					continue;
				}
				start = reserved_cap;
				break;
			}
#endif 

#define BSP_SPECIAL_HANDLING
#ifdef BSP_SPECIAL_HANDLING
			{
				int ccpu = ffsll_bitmask(&mask) -1;
				if (ccpu == 0) {
					//consider the cap has part of the BSP memory
					memres * pamem = amemres;
					long long end = 0;
					long long total_to_sum = 0;
					//sum up all holes to the cap
					while ( pamem != smemres ) {
						total_to_sum += (pamem->start - end);
						end = pamem->end;
						pamem++;
					}
					total_to_sum += (pamem->start - end);

					printf("cpu %d sum of hole mem before the cap %lld %llx\n", ccpu, total_to_sum, total_to_sum);
					//This memory will be added to the total available to obtain the perfect chunk size in order to allocate the first 16MB
					size += total_to_sum;
					// recalculate the size
					alignedchunk =  chunk & ~RESOLUTION_MASK;
					new_total = alignedchunk * cpu_num;
					diff = size -new_total;
					start = 0;
				}
			}
#endif

			for (l=0; l<cpu_num; l++) {
				int ccpu = ffsll_bitmask(&mask) -1;
#ifdef BEN_ALIGNMENT
				if ( (start & (unsigned long long)~((unsigned long long)BEN_ALIGNMENT -1)) ) {
					start &= (unsigned long long)~((unsigned long long)BEN_ALIGNMENT -1);
					start += BEN_ALIGNMENT;
				}
#endif
				long long endend = start + alignedchunk;
				long long avail = (smemres->end - start);
				long long required = alignedchunk, total = alignedchunk;
				//hole handling code
				while ( avail < required ) {
					//switch to next resource
					total += (smemres +1)->start - smemres->end;
					smemres += 1;
					required -= avail;
					avail = smemres->end - smemres->start;
					endend = smemres->start + required;
				}

#ifdef BSP_SPECIAL_HANDLING
				if (ccpu == 0)
					printf ("present_mask=%d memmap=%ldM@%ldM mem=%ldM\n",
							ccpu,
							(unsigned long)total >> 20, (unsigned long)start >> 20,
							(unsigned long)(endend) >> 20
					);
				else
#endif
					printf ("present_mask=%d memmap=%ldM@%ldM memmap=%ldM$%ldM mem=%ldM\n",
							ccpu,
							(unsigned long)total >> 20, (unsigned long)start >> 20,
							(unsigned long)(start - reserved_cap) >> 20, (unsigned long)reserved_cap >> 20,
							(unsigned long)(endend) >> 20
					);
				start = endend;

				clearcpu_bitmask(&mask, ccpu);
			}
		}
	}
	return 1;
}

int partitionedcpu_globalshm_nonodes ( numa_node * list)
{
#define COM_RESERVOIR  
#ifdef COM_RESERVOIR
#define MACH_64CORE_RESERV 0x70000000
	long long reserved_com = (0x40 << 20) + MACH_64CORE_RESERV; // 64MB for application communication (Arijit)
#endif

	//do the partition without caring about nodes
	//long long reserved_cap = (0x10 << 20);   //use 16MB reservation at the beginning
	long long reserved_cap = (0x1 << 20);   //use 1MB reservation at the beginning
	int cpu_num = (numa_configured_cpus() +1);
	//  int size = maxpresentmem;
	unsigned long long  size = total_by_node;

	// we do not care about nodes here
	memres * smemres = amemres;
	memres * spcires = apcires;
	long long start = smemres->start;
	long long startpci = spcires->start;

	printf("##### %s ##### \n", __func__);

	//contains reserved_cap?
	while ( smemres->start < reserved_cap ) {
		if (smemres->end < reserved_cap) { // the hole is inside the cap
			smemres += 1;
			start = smemres->start;
			continue;
		}
		start = reserved_cap;
		break;
	}
	while ( spcires->end < start)
		spcires +=1;
	if (spcires->start < start && start < spcires->end) {
		start = spcires->end;
		printf("WARNING PCI hole around 0x%lx (reserved_cap)\n", reserved_cap);
	}
	/*printf("%s: INFO reserved_cap %lx mem %lx-%lx was %lx-%lx pci %lx-%lx\n",
		__func__, reserved_cap, start, smemres->end,
		smemres->start, smemres->end, spcires->start, spcires->end);*/
	memres * papci = spcires;

	// first cpu we consider is always zero
	//consider the cap has part of the BSP memory
	memres * pamem = amemres;
	long long end = 0;
	long long total_to_sum = 0;
	//sum up all holes to the cap
	while ( pamem != smemres ) {
		total_to_sum += (pamem->start - end);
		end = pamem->end;
		pamem++;
	}
	total_to_sum += (pamem->start - end);

	printf("NONODES cpu %d sum of hole mem before the cap %lld %llx\n", cpu_num, total_to_sum, total_to_sum);
	//This memory will be added to the total available to obtain the perfect chunk size in order to allocate the first 16MB
	size += total_to_sum;

	unsigned long long chunk = 0;
	unsigned long long alignedchunk =0;
	long long new_total = -1;

	//#define BEN_ALIGN_CHUNK 0x40000000
#define BEN_ALIGN_CHUNK 0x8000000
#ifdef BEN_ALIGN_CHUNK 
	//this must run on the available memory: a lot of memory will be lost because of these alignments
	unsigned long chunky_num = size /(unsigned long long) BEN_ALIGN_CHUNK;
	unsigned long chunky_per_cpu = chunky_num / cpu_num;
	printf("chunkys num = %ld integer chunks per cpu = %ld wasted chunks = %ld\n",
			chunky_num, chunky_per_cpu, (chunky_num - (chunky_per_cpu * cpu_num)) );

	chunk = (unsigned long long)chunky_per_cpu * (unsigned long long)BEN_ALIGN_CHUNK;
	alignedchunk =  chunk & ~RESOLUTION_MASK;
	new_total = alignedchunk * cpu_num;
#else 
	// total size divided by the number of cpu
	chunk = size / cpu_num;
	// recalculate the size
	alignedchunk =  chunk & ~RESOLUTION_MASK;
	new_total = alignedchunk * cpu_num;
#endif

#ifdef COM_RESERVOIR
	size -= reserved_com;
#endif

	long long diff = size -new_total;
	start = 0;

	printf("cpus: %d last address 0x%llx avail memory: 0x%llx (mem lost 0x%llx) chunks %llx aligned %llx\n",
			cpu_num , list[maxconfigurednode].rend->end, size,
			list[maxconfigurednode].rend->end - size,
			chunk, alignedchunk
	);

#ifdef BEN_ALIGN_CHUNK
	{
		//realign in Ben's required fashion all the memory, i.e. chunks of 512MB, so rewrite the memory map
		// start to ceil, end to floor
		memres * a = smemres;
		while ((a->start != 0) && (a->end !=0)) {
			printf("REALIGN MEM WAS: 0x%llx - 0x%llx *** ", a->start, a->end);
			if ( (a->start > reserved_cap ) && (a->start & (BEN_ALIGN_CHUNK -1)) ) { // ceil
				a->start &= ~(BEN_ALIGN_CHUNK -1);
				a->start += BEN_ALIGN_CHUNK;
			}
			if ( (a->end & (BEN_ALIGN_CHUNK -1)) ) { // floor
				a->end &= ~(BEN_ALIGN_CHUNK -1);
			}
			printf("REALIGN MEM  IS: 0x%llx - 0x%llx\n", a->start, a->end);
			a++;
		}
	}
#endif

	int l;
	for (l=0; l<cpu_num; l++) {
		long long endend = start + alignedchunk;
		long long avail = (smemres->end - start);
		long long required = alignedchunk, total = alignedchunk;
		memres * spcitmp = papci;

#ifdef COM_RESERVOIR
		printf("vty_offset=0x%llx ", (list[maxconfigurednode].rend->end - reserved_com) );
#endif
		printf ("present_mask=%d ", l);

		//hole handling code - todo follow with pci map
		while ( avail < required ) {
			//switch to next resource
			total += (smemres +1)->start - smemres->end;
			smemres += 1;
			required -= avail;
			avail = smemres->end - smemres->start;
			endend = smemres->start + required;
		}
		
		long long curr_start = start;
		long long curr_total = total;
		while ((spcires->end < (total + start)) && !(spcires->start==0 && spcires->end==0)) {
//printf("PCI %lx-%lx MAX %lx start %lx total %lx\n", spcires->start, spcires->end , (total + start), curr_start, curr_total);
			if (spcires->start != curr_start) { //note that PCI maps of root level seems to be coalesced around the PCI hole under the 4GB
				curr_total = curr_total - (spcires->start - curr_start);
				printf ("memmap=%ldM@%ldM ", (unsigned long)(spcires->start - curr_start) >> 20, (unsigned long)curr_start >> 20);
			}
			curr_total = curr_total - (spcires->end -spcires->start);
			curr_start = spcires->end;
			spcires += 1;
		}
		printf ("memmap=%ldM@%ldM ", (unsigned long)curr_total >> 20, (unsigned long)curr_start >> 20); // original

		if (l != 0) {
			long long res_start = reserved_cap;
			long long res_total = (start - reserved_cap);
			while ((spcitmp->end < start) && !(spcitmp->start==0 && spcitmp->end==0)) {
//printf("PCI holes %lx-%lx MAX %lx start %lx total %lx\n", spcitmp->start, spcitmp->end , start, res_start, res_total);
	                        if (spcitmp->start != res_start) { //note that PCI maps of root level seems to be coalesced around the PCI hole under the 4GB
        	                        res_total = res_total - (spcitmp->start - res_start);
                	                printf ("memmap=%ldM$%ldM ", (unsigned long)(spcitmp->start - res_start) >> 20, (unsigned long)res_start >> 20);
                        	}
	                        res_total = res_total - (spcitmp->end -spcitmp->start);
	                        res_start = spcitmp->end;
				spcitmp += 1;
			}
			printf ("memmap=%ldM$%ldM ", (unsigned long)(res_total) >> 20, (unsigned long)res_start >> 20);
		}

		printf ("mem=%ldM\n", (unsigned long)(endend) >> 20);
		start = endend;
	}

	return 1;
}

// clustered around the nodes
int clusteredcpu_on_nodes ( numa_node * list)
{

#define COM_RESERVOIR
#ifdef COM_RESERVOIR
#define MACH_64CORE_RESERV 0x70000000
	long long reserved_com = (0x40 << 20) + MACH_64CORE_RESERV; // 64MB for application communication (Arijit)
#endif


	//do the partition per cluster
	// instead of asking the cluster size (that can be done in the future if required..)
	// we just need to get the cpu per node and cluster on them,

	long long reserved_cap = (0x1 << 20);   //use 16MB reservation at the beginning

	// 512 MB
#define BEN_ALIGNMENT 0x20000000
#ifdef BEN_ALIGNMENT
	printf ("CLUSTERED Ben ALIGNMENT memmap=x@ALIGNED buond %lld\n", (long long) BEN_ALIGNMENT);
#endif
	printf("vty_offset not fully supported: NOT ALL CLUSTERS WILL WORK\n");
	printf("##### %s #####\n", __func__);

	// algorithm is: if over 16MB
	int i, id_base =0;
	long long start =0;

	for (i=0 ; i  < (maxconfigurednode +1) ; i++) {
		int cpu_num = list[i].cpus;
		if (cpu_num == 0) {
			printf("ERROR i %d cpu_num %d\n",i , cpu_num );
			return -1;
		}

		long long chunk = list[i].size;
		long long alignedchunk =  chunk & ~RESOLUTION_MASK;
		//		long long diff = size -new_total;
		long long start = list[i].start;

#ifdef COM_RESERVOIR
		printf("vty_offset=0x%llx ", (list[maxconfigurednode].rend->end - reserved_com) );
#endif
		if (start < reserved_cap) { // include reserved cap
			printf ("present_mask=%d-%d memmap=%ldM@%ldM mem=%ldM\n",
					id_base, id_base + (cpu_num -1),
					(unsigned long)alignedchunk >> 20, (unsigned long)start >> 20,
					(unsigned long)(alignedchunk) >> 20);
		} else {
			printf ("present_mask=%d-%d memmap=%ldM@%ldM memmap=%ldM$%ldM mem=%ldM\n",
					id_base, id_base + (cpu_num -1),
					(unsigned long)alignedchunk >> 20, (unsigned long)start >> 20,
					(unsigned long)(start - reserved_cap) >> 20, (unsigned long)reserved_cap >> 20,
					(unsigned long)(start + alignedchunk) >> 20);
		}

		id_base += cpu_num;
	}
	return 1;
}


///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{ 
	/* options
	 * -p <number_of_partitions>
	 * -f <fixed_amount_of_memory_per_partition> this is special option, otherwise use all the available memory (per partition)
	 * -b <not_allocable_area> this is 16MB by default
	 */

	// output will be
	// present_mask=1,2,3,4,5 mem=3G memmap=16MB$0


	// TODO

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

	long long total = 0, size = 0, cmap = 0;
	int i;
	for (i=0; i< (maxconfigurednode +1); i++) {
		size = numa_node_size64(i, 0);
		cpu_bitmask_t a;
		numa_node_cpumask(i, &a);
		total += size;
		printf("node %d mem %lld %llx map TODOllx (%d core)\n",
				i, size, size, bit_weight_bitmask(&a));

		// print_bitmask(&a); printf("\n");
	}

	printf ("total = %lld %lld kB %lld MB %lld GB\n\n", total, total >>10, total >>20, total >>30);
	total_by_node = total; //unsigned long long total_by_node=-1;
	/////////////////////////////////////////////////////////////////////////////

	set_present_mem();
	set_pci_holes();

	int im= -1;
	for (i=0; i<MAX_AMEMRES; i++)
		if ((amemres[i].start == 0) && (amemres[i].end == 0))
			break;
	im = i -1;
	int ip= -1;
	for (i=0; i<MAX_AMEMRES; i++)
		if ((apcires[i].start == 0) && (apcires[i].end == 0))
			break;
	ip = i -1;
	check_intersections(amemres, im+1, apcires, ip+1);

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

	// the following is outputted from linu/arch/x86/mm/numa.c in  setup_node_data
	printf("\n\nCROSSCHECK for consistency with\n"
			"dmesg | grep NODE_DATA\n"
			"dmesg | grep \"Initmem setup node\"\n\n");

	//--------------------------------------------------------------------------------

	// precheck - we cannot have more then num_cpu partitions -p

	// here the memory allocator has to run , different memory allocators can be written
	// policy 0: how many partitions? 1, #proc or how many?!
	// policy 1: number of processor (everyone or subset)

	// policy 2: single shared memory area, (fixed)
	// policy 3: per node shared memory memory area in mklinux

	// policy 4:

	// partitionedcpu_globalshm_nonodes ( anode);
	// partitionedcpu_globalshm( anode);
	partitionedcpu_globalshm_nonodes ( anode);
	// clusteredcpu_on_nodes(anode);

	free(anode);

	return 0;
}




