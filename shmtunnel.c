// shmtunnel.c
// Shelton's modified version of tunnel.c to support shmtunnel driver in krn
// this application only open the shared memory tunnel is not doing anything else
// (cleaned by Antonio and Saif c 2013)

/* tunnel.c is used to attach a TUN/TAP interface between two kernels
   through shared memory. */

#include <net/if.h>
#include <linux/if_tun.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sched.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>


/* 
 * Allocate TUN device, returns opened fd. 
 * Stores dev name in the first arg(must be large enough).
 */  
static int shmtun_open_common0(char *dev)
{
	char tunname[17];
	int i, fd, err;

	if (*dev) {
		sprintf(tunname, "/dev/%s", dev);
		return open(tunname, O_RDWR);
	}

	sprintf(tunname, "/dev/shmtun");
	err = 0;
	for (i=0; i < 255; i++) {
		sprintf(tunname + 11, "%d", i);
		/* Open device */
		if ((fd=open(tunname, O_RDWR)) > 0) {
			strcpy(dev, tunname + 5);
			return fd;
		}
		else if (errno != ENOENT)
			err = errno;
		else if (i)	/* don't try all 256 devices */
			break;
	}
	if (err)
		errno = err;
	return -1;
}

#ifndef OTUNSETNOCSUM
/* pre 2.4.6 compatibility */
#define OTUNSETNOCSUM  (('T'<< 8) | 200) 
#define OTUNSETDEBUG   (('T'<< 8) | 201) 
#define OTUNSETIFF     (('T'<< 8) | 202) 
#define OTUNSETPERSIST (('T'<< 8) | 203) 
#define OTUNSETOWNER   (('T'<< 8) | 204)
#endif

static int shmtun_open_common(char *dev)
{
	struct ifreq ifr;
	int fd;

	if ((fd = open("/dev/net/shmtun", O_RDWR)) < 0) {
		printf("Calling shmtun_open_common0...\n");
		return shmtun_open_common0(dev);
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	if (*dev)
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {

		printf("ioctl failed, errno = %d\n", errno);

		if (errno == EBADFD) {
			/* Try old ioctl */
			if (ioctl(fd, OTUNSETIFF, (void *) &ifr) < 0) 
				goto failed;
		} else
			goto failed;
	} 

	strcpy(dev, ifr.ifr_name);

	printf("ifr_name: %s\n", ifr.ifr_name);

	return fd;

failed:
	close(fd);
	return -1;
}

#define MAX_IP 64
#define MAX_VERBOSE 0x8000
#define MAX_BUFFER 0x1000
#define MAGIC_NUMBER 0xA5A5C3C3
#define STATUS_CON 0x12345678
#define STATUS_DISCON 0x87654321
#define NUM_SPIN 0x1000

typedef struct ip_tunnel {
	int magic;
	int status;
	int lock;
	int content_size;
	char buffer [MAX_BUFFER];
} ip_tunnel_t;

int tun_open(char *dev) { return shmtun_open_common(dev); }

int tun_close(int fd, char *dev) { return close(fd); }
int tap_close(int fd, char *dev) { return close(fd); }

typedef struct __popcorn{
	int fd;
	int cpu;
	ip_tunnel_t * addr;
} popcorn;

void dump(ip_tunnel_t *data, int max) {
	int i;
	int mem_fd;
	void* physical;  

	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (mem_fd < 0) {
		perror("open /dev/mem error");
		exit(0);
	}

	long delta = sysconf(_SC_PAGE_SIZE);

	printf("mmap(%p, %ld, 0x%x, 0x%x, %d, 0x%lx)\n",
			(void*) 0, (sizeof(ip_tunnel_t) * MAX_IP), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (unsigned long)data); 
	//(void*) 0, (delta * 4), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (off_t)addr); 
	physical = mmap(0, (sizeof(ip_tunnel_t) * MAX_IP), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (off_t)data);
	//physical = mmap(0, delta * 4, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (off_t)addr);
	printf("base address %p\n", physical);
	if ((unsigned long) physical == -1) {
		perror("mmap");
		exit(0);
	}

	data = physical;
	for (i = 0; i < max; i++)
		printf("%d: m:%x s:%s l:%d i:%d\n", i,
				data[i].magic,
				(data[i].status == STATUS_CON) ? "conn" : (data[i].status == STATUS_DISCON) ? "deco" : "none",
				data[i].lock,
				data[i].content_size);
}

int main (int argc, char * argv [] )
{
	char name[32];
	memset(name, 0, 32);

	if (argc == 2) {
		void * phy_addr;
		phy_addr = (void*)strtoul(argv[1], 0, 0);

		dump(phy_addr, MAX_IP);
		return 0;
	}
	if (argc != 3)
		printf("usage: tun physical_addr cpuid\n");

	int tun = tun_open(name);
	void * phy_addr;
	phy_addr = (void*)strtoul(argv[1], 0, 0);
	int cpuid = atoi(argv[2]);

	if (!(cpuid < MAX_IP)) {
		printf("ERROR cpuid (%d) greater then MAX_IP (%d)\n", cpuid, MAX_IP);
		exit(0);
	}
	printf("tun %s (fd %d)\n", name, tun);

	// TODO Handling CTRL+C
	while (1)
		sleep(1200);

	printf("shmtunnel end\n");
	return 0;
}
