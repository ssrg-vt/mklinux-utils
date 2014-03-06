// in order to get some informations from the adopted glibc
// Copyright (c) Antonio Barbalace, SSRG, VT 2014

// NOTE this file must be statically linked with glibc

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

int main (int argc, char* argv)
{
  extern size_t _dl_tls_static_size;
_dl_tls_static_align
_dl_tls_max_dtv_idx
size_t __static_tls_size;
size_t __static_tls_align_m1;

extern int * __libc_multiple_threads_ptr;

	  __pthread_multiple_threads = *__libc_multiple_threads_ptr = 1;
  printf("_dl_tls_static_size %ld\n", (unsigned long)_dl_tls_static_size);
 
__fork_handlers
__nptl_nthreads
 __libc_multiple_threads_ptr
  {
    size_t __kernel_cpumask_size=0;
    int res;
    size_t psize = 8;
    void *p = malloc (psize);
    memset(p, 0xFF, psize);

    while ( (res = syscall (__NR_sched_getaffinity, 0, psize, p)) < 0 ) {
      psize = 2*psize;
      p = (void*) realloc (p, psize);
      memset(p, 0xFF, psize);
    }
    free(p);

      __kernel_cpumask_size = psize;
      printf("__kernel_cpumask_size is %d\n", psize);

  }
}
  
0  EXTERN struct dtv_slotinfo_list
371  {
372    size_t len;
373    struct dtv_slotinfo_list *next;
374    struct dtv_slotinfo
375    {
376      size_t gen;
377      struct link_map *map;
378    } slotinfo[0];
379  } *_dl_tls_dtv_slotinfo_list;