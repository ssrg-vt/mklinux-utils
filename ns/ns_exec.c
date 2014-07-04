/* ns_exec.c 

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Join a namespace and execute a command in the namespace
*/
#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h> /* should include setns */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>

/* A simple error-handling function: print an error message based
   on the value in 'errno' and terminate the calling process */

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

#ifndef __NR_sys_setns
#define __NR_sys_setns 308
#endif

#ifndef setns
static inline int setns(int fd, int nstype)
{
    return syscall(__NR_sys_setns, fd, nstype);
}
#endif

int
main(int argc, char *argv[])
{
    int fd; int i;
    char *fmtent[] = {"ipc", "uts", "net", "pid"};
    char *fmtstr = "/proc/%s/ns/%s";
    char fmtbuf[64];

//printf("setns: %d\n", __NR_sys_setns);

    if (argc < 3) {
        fprintf(stderr, "%s PID cmd [arg...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

for (i=0; i<4; i++) {    
    sprintf(fmtbuf, fmtstr, argv[1], fmtent[i]);
printf("attaching to %s\n", fmtbuf);
    fd = open(fmtbuf, O_RDONLY);   /* Get descriptor for namespace */
    if (fd == -1)
        errExit("open");

    if (setns(fd, 0) == -1)         /* Join that namespace */
        errExit("setns");
//    close(fd);
};

    execvp(argv[2], &argv[2]);      /* Execute a command in namespace */
    errExit("execvp");
}
