/**
 * \file
 * \brief Implementation of backend functions on Linux
 */

// Popcorn version
// Copyright Antonio Barbalace, SSRG, VT, 2013

/*
 * Copyright (c) 2007, 2008, 2009, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
//#include <numa.h>
#include <sched.h>
//#include <pthread.h>

#include <unistd.h>
#include <sys/syscall.h>

#include "backend.h"
#include "omp.h"

#define offsetof(S,F) ((size_t) & (((S *) 0)->F))

/* the following code is extracted from glibc */
#ifndef __cpu_set_t_defined
# define __cpu_set_t_defined
/* Size definition for CPU sets.  */
# define __CPU_SETSIZE	1024
# define __NCPUBITS	(8 * sizeof (__cpu_mask))

/* Type for array elements in 'cpu_set_t'.  */
typedef unsigned long int __cpu_mask;

/* Basic access functions.  */
# define __CPUELT(cpu)	((cpu) / __NCPUBITS)
# define __CPUMASK(cpu)	((__cpu_mask) 1 << ((cpu) % __NCPUBITS))

/* Data structure to describe CPU mask.  */
typedef struct
{
	  __cpu_mask __bits[__CPU_SETSIZE / __NCPUBITS];
} cpu_set_t;

# if __GNUC_PREREQ (2, 91)
#  define __CPU_ZERO_S(setsize, cpusetp) \
	  do __builtin_memset (cpusetp, '\0', setsize); while (0)
# else
#  define __CPU_ZERO_S(setsize, cpusetp) \
	  do {									      \
		      size_t __i;								      \
		      size_t __imax = (setsize) / sizeof (__cpu_mask);			      \
		      __cpu_mask *__bits = (cpusetp)->__bits;				      \
		      for (__i = 0; __i < __imax; ++__i)					      \
		        __bits[__i] = 0;							      \
		    } while (0)
# endif
# define __CPU_SET_S(cpu, setsize, cpusetp) \
	  (__extension__							      \
	      ({ size_t __cpu = (cpu);						      \
	             __cpu < 8 * (setsize)						      \
	             ? (((__cpu_mask *) ((cpusetp)->__bits))[__CPUELT (__cpu)]		      \
			     	 |= __CPUMASK (__cpu))						      \
	             : 0; }))

# define CPU_ZERO(cpusetp)	 __CPU_ZERO_S (sizeof (cpu_set_t), cpusetp)
# define CPU_ZERO_S(setsize, cpusetp)	    __CPU_ZERO_S (setsize, cpusetp)
#endif

/* the following code is extracted from glibc */
// /sysdeps/unix/sysv/linux/sched_setaffinty.c 
#ifndef __NR_sched_setaffinity
  #error "syscall sched_setaffinity not defined"
#endif

static size_t __kernel_cpumask_size = 0;
int __sched_setaffinity (pid_t pid, size_t cpusetsize, const cpu_set_t *cpuset)
{
  if (__kernel_cpumask_size == 0 )
  {
    int res;
    size_t psize = 8;
    void *p = malloc (psize);
    memset(p, 0xFF, psize);

    while ( (res = syscall (__NR_sched_setaffinity, pid, psize, p)) < 0 ) {
      psize = 2*psize;
      p = (void*) realloc (p, psize);
      memset(p, 0xFF, psize);
      printf("try %d %d\n", psize, res);
    }

      __kernel_cpumask_size = psize;
      printf("__kernel_cpumask_size is %d\n", psize);
  }

  /* We now know the size of the kernel cpumask_t. 
   * Make sure the user does not request to set a bit beyond that.  */
  size_t cnt;
  for (cnt = __kernel_cpumask_size; cnt < cpusetsize; ++cnt)
    if (((char *) cpuset)[cnt] != '\0')
      {
        /* Found a nonzero byte.  This means the user request cannot be
	fulfilled.  */
	return -1;
      }

  int result = syscall (__NR_sched_setaffinity, pid, cpusetsize, cpuset);

  return result;
}

#define PTHREAD_KEYS_MAX 8
/* We keep thread specific data in a special data structure, a two-level
   array.  The top-level array contains pointers to dynamically allocated
   arrays of a certain number of data pointers.  So we can implement a
   sparse array.  Each dynamic second-level array has
        PTHREAD_KEY_2NDLEVEL_SIZE
   entries.  This value shouldn't be too large.  */
#define PTHREAD_KEY_2NDLEVEL_SIZE       32
/* We need to address PTHREAD_KEYS_MAX key with PTHREAD_KEY_2NDLEVEL_SIZE
   keys in each subarray.  */
#define PTHREAD_KEY_1STLEVEL_SIZE \
  ((PTHREAD_KEYS_MAX + PTHREAD_KEY_2NDLEVEL_SIZE - 1) \
   / PTHREAD_KEY_2NDLEVEL_SIZE)

/* Thread-local data handling.  */
struct pthread_key_struct
{
  /* Sequence numbers.  Even numbers indicated vacant entries.  Note
     that zero is even.  We use uintptr_t to not require padding on
     32- and 64-bit machines.  On 64-bit machines it helps to avoid
     wrapping, too.  */
  uintptr_t seq;

  /* Destructor for the data.  */
  void (*destr) (void *);
};

//from glibc/nptl/descr.h
//in struct pthread
struct backend {
  void * self;
  struct backend_key_data
  {
    /* Sequence number.  We use uintptr_t to not require padding on
       32- and 64-bit machines.  On 64-bit machines it helps to avoid
       wrapping, too.  */
    uintptr_t seq;

    /* Data pointer.  */
    void *data;
  } specific_1stblock[PTHREAD_KEY_2NDLEVEL_SIZE];

  /* Two-level array for the thread-specific data.  */
  struct backend_key_data *specific[PTHREAD_KEY_1STLEVEL_SIZE];
  
  /* Flag which is set when specific data is set.  */
  char specific_used;
};

/* 1 if 'type' is a pointer type, 0 otherwise.  */
# define __pointer_type(type) (__builtin_classify_type ((type) 0) == 5)

/* __intptr_t if P is true, or T if P is false.  */
# define __integer_if_pointer_type_sub(T, P) \
  __typeof__ (*(0 ? (__typeof__ (0 ? (T *) 0 : (void *) (P))) 0 \
		  : (__typeof__ (0 ? (__intptr_t *) 0 : (void *) (!(P)))) 0))

/* __intptr_t if EXPR has a pointer type, or the type of EXPR otherwise.  */
# define __integer_if_pointer_type(expr) \
  __integer_if_pointer_type_sub(__typeof__ ((__typeof__ (expr)) 0), \
				__pointer_type (__typeof__ (expr)))

/* Cast an integer or a pointer VAL to integer with proper type.  */
# define cast_to_integer(val) ((__integer_if_pointer_type (val)) (val))

/* Loading addresses of objects on x86-64 needs to be treated special
  when generating PIC code.  */
#undef __pic__
#ifdef __pic__
# define IMM_MODE "nr"
#else
# define IMM_MODE "ir"
#endif

/* Same as THREAD_SETMEM, but the member offset can be non-constant.  */
# define THREAD_SETMEM(descr, member, value) \
  ({ if (sizeof (descr->member) == 1)					      \
       asm volatile ("movb %b0,%%fs:%P1" :				      \
		     : "iq" (value),					      \
		       "i" (offsetof (struct backend, member)));	      \
     else if (sizeof (descr->member) == 4)				      \
       asm volatile ("movl %0,%%fs:%P1" :				      \
		     : IMM_MODE (value),				      \
		       "i" (offsetof (struct backend, member)));	      \
     else								      \
       {								      \
	 if (sizeof (descr->member) != 8)				      \
	   /* There should not be any value with a size other than 1,	      \
	      4 or 8.  */						      \
	   abort ();							      \
									      \
	 asm volatile ("movq %q0,%%fs:%P1" :				      \
		       : IMM_MODE ((uint64_t) cast_to_integer (value)),	      \
			 "i" (offsetof (struct backend, member)));	      \
       }})
       
/* Set member of the thread descriptor directly.  */
# define THREAD_SETMEM_NC(descr, member, idx, value) \
  ({ if (sizeof (descr->member[0]) == 1)				      \
       asm volatile ("movb %b0,%%fs:%P1(%q2)" :				      \
		     : "iq" (value),					      \
		       "i" (offsetof (struct backend, member[0])),	      \
		       "r" (idx));					      \
     else if (sizeof (descr->member[0]) == 4)				      \
       asm volatile ("movl %0,%%fs:%P1(,%q2,4)" :			      \
		     : IMM_MODE (value),				      \
		       "i" (offsetof (struct backend, member[0])),	      \
		       "r" (idx));					      \
     else								      \
       {								      \
	 if (sizeof (descr->member[0]) != 8)				      \
	   /* There should not be any value with a size other than 1,	      \
	      4 or 8.  */						      \
	   abort ();							      \
									      \
	 asm volatile ("movq %q0,%%fs:%P1(,%q2,8)" :			      \
		       : IMM_MODE ((uint64_t) cast_to_integer (value)),	      \
			 "i" (offsetof (struct backend, member[0])),	      \
			 "r" (idx));					      \
       }})       

/* Same as THREAD_GETMEM, but the member offset can be non-constant.  */
# define THREAD_GETMEM_NC(descr, member, idx) \
  ({ typeof (descr->member[0]) __value;				      \
     if (sizeof (__value) == 1)						      \
       asm volatile ("movb %%fs:%P2(%q3),%b0"				      \
		     : "=q" (__value)					      \
		     : "0" (0), "i" (offsetof (struct backend, member[0])),   \
		       "r" (idx));					      \
     else if (sizeof (__value) == 4)					      \
       asm volatile ("movl %%fs:%P1(,%q2,4),%0"				      \
		     : "=r" (__value)					      \
		     : "i" (offsetof (struct backend, member[0])), "r" (idx));\
     else								      \
       {								      \
	 if (sizeof (__value) != 8)					      \
	   /* There should not be any value with a size other than 1,	      \
	      4 or 8.  */						      \
	   abort ();							      \
									      \
	 asm volatile ("movq %%fs:%P1(,%q2,8),%q0"			      \
		       : "=r" (__value)					      \
		       : "i" (offsetof (struct backend, member[0])),	      \
			 "r" (idx));					      \
       }								      \
    __value; })

//from nptl/sysdeps/unix/sysv/linux/internaltypes.h
/* Check whether an entry is unused.  */
#define KEY_UNUSED(p) (((p) & 1) == 0)
#define KEY_USABLE(p) (((uintptr_t) (p)) < ((uintptr_t) ((p) + 2)))

// from /bits/atomic.h
#define atomic_compare_and_exchange_bool_acq(mem, newval, oldval) \
  ({ __typeof (mem) __gmemp = (mem);				      \
     __typeof (*mem) __gnewval = (newval);			      \
								      \
     *__gmemp == (oldval) ? (*__gmemp = __gnewval, 0) : 1; })

/* Keys for thread-specific data */
typedef unsigned int backend_key_t;

/* Table of the key information.  */
static struct pthread_key_struct ____pthread_keys[PTHREAD_KEYS_MAX];

int ____pthread_key_create (backend_key_t *key, void (*destr) (void *))
{
  /* Find a slot in __pthread_kyes which is unused.  */
  size_t cnt;
  for (cnt = 0; cnt < PTHREAD_KEYS_MAX; ++cnt)
    {
      uintptr_t seq = ____pthread_keys[cnt].seq;

      if (KEY_UNUSED (seq) && KEY_USABLE (seq)
	  /* We found an unused slot.  Try to allocate it.  */
	  && ! atomic_compare_and_exchange_bool_acq (&____pthread_keys[cnt].seq,
						     seq + 1, seq))
	{
	  /* Remember the destructor.  */
	  ____pthread_keys[cnt].destr = destr;
	  /* Return the key to the caller.  */
	  *key = cnt;
	  /* The call succeeded.  */
	  return 0;
	}
    }

  return EAGAIN;
}

static backend_key_t backend_key;

void backend_init(void)
{
    int r = ____pthread_key_create(&backend_key, NULL);
    if (r != 0) {
        printf("pthread_key_create failed\n");
    }
}

/* NOTE: struct pthread's first entry is a tcbhead_t
 * from glibc/nptl/descr.h
 * struct pthread
 * {
 *   union
 *   {
 * #if !TLS_DTV_AT_TP
 *   tcbhead_t header;
 */
/* Return the thread descriptor for the current thread.

   The contained asm must *not* be marked volatile since otherwise
   assignments like
   pthread_descr self = thread_self();
   do not get optimized away.  */
// struct pthread is a union
// header.self vedi glibc/nptl/sysdeps/x86_64/tls.h
# define THREAD_SELF \
  ({ struct backend *__self;						      \
     asm ("mov %%fs:%c1,%0" : "=r" (__self)				      \
	  : "i" (offsetof (struct backend, self)));	 	      \
     __self;})

void backend_set_numa(unsigned id)
{
/*
    struct bitmask *bm = numa_allocate_cpumask();
    numa_bitmask_setbit(bm, id);
    numa_sched_setaffinity(0, bm);
    numa_free_cpumask(bm);
*/
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(id, &cpu_mask);
    //sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
    __sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
}

typedef struct _backend_args {
  void * user;
  int (* cfunc)(void * args);
} backend_args;

int backend_start_func (void * args)
{
  int ret =
    ((backend_args *)args)->cfunc(((backend_args *) args)->user);
  printf("%s: barg %p carg %p cfunc %p\n",
    __func__,  args, ((backend_args *)args)->cfunc,((backend_args *) args)->user);
  free( args );
  
  syscall (__NR_exit, ret); // <--- to simulate pthread
  /* never reaching here */
  return -1;
}

// we assume stack is growing downward (tipical in x86)
#define STACK_SIZE 0x1000
// from glibc
#define CLONE_SIGNAL            (CLONE_SIGHAND | CLONE_THREAD)

void backend_run_func_on(int core_id, void* cfunc, void *arg)
{
//    pthread_t pthread;
  int r = 0; // pthread_create(&pthread, NULL, cfunc, arg);
#define GLIBC_STYLE 1
#if GLIBC_STYLE      
  int clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGNAL
		     | CLONE_SETTLS | CLONE_PARENT_SETTID
		     //| CLONE_PARENT_SETTID
		     | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM
#if __ASSUME_NO_CLONE_DETACHED == 0
		     | CLONE_DETACHED
#endif
		     | 0);
#else
   int clone_flags = (CLONE_THREAD|CLONE_SIGHAND|CLONE_VM); 
#endif
    struct backend * backendptr;
    pid_t ptid, ctid;
    void * stack = malloc(STACK_SIZE);
    if (!stack) {
	printf("stack allocation error");
	exit (0);
    }
    backendptr = malloc(sizeof(sizeof(struct backend)));
    if (!backendptr) {
	printf("TLS allocation error");
	exit (0);
    }
    backendptr->self = backendptr;
    backend_args * bkargs = malloc(sizeof(backend_args));
    if (!bkargs) {
	printf("args allocation error");
	exit (0);      
    }
    bkargs->user = arg;
    bkargs->cfunc = cfunc;
    /* NOTE: tls value is struct pthread *
     * in glibc/nptl/sysdeps/pthread/createthread.c
     * do_clone (struct pthread *pd, const struct pthread_attr * attr,
     *     int clone_flags, int (*fct) (void*), STACK_VARIABLES_PARMS, int stopped)
     * :75 int rc = ARCH_CLONE (fct, STACK_VARIABLES_ARGS, clone_flags,
     *                   pd, &pd->tid, pd, &pd->tid);
     */    
    //r = clone(cfunc, (stack+STACK_SIZE), clone_flags, arg, &ptid, 0, &ctid);
    //r = clone(cfunc, (stack+STACK_SIZE), clone_flags, arg, &ptid, backend_t, &ctid);
    //r = clone(backend_start_func, (stack+STACK_SIZE), clone_flags, bkargs, &ptid, 0, &ctid);
    printf ("%s: cpuid %d stack %p tls %p barg %p carg %p bfunc %p cfunc %p\n",
	    __func__, core_id, 
	    (stack+STACK_SIZE), backendptr, 
	    bkargs, arg, backend_start_func, cfunc);
    r = clone(backend_start_func, (stack+STACK_SIZE), clone_flags, bkargs, &ptid, backendptr, &ctid);
    if (r == -1) {
        //printf("clone failed\n");
	perror("clone failed");
	exit (0);
    }
}

// from glibc
void * ____pthread_getspecific (backend_key_t key)
{
  struct backend_key_data *data;

  /* Special case access to the first 2nd-level block.  This is the
     usual case.  */
  if ( key < PTHREAD_KEY_2NDLEVEL_SIZE )
    data = &THREAD_SELF->specific_1stblock[key];
  else
    {
      /* Verify the key is sane.  */
      if (key >= PTHREAD_KEYS_MAX)
	/* Not valid.  */
	return NULL;

      unsigned int idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;
      unsigned int idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;

      /* If the sequence number doesn't match or the key cannot be defined
	 for this thread since the second level array is not allocated
	 return NULL, too.  */
      struct backend_key_data *level2 = THREAD_GETMEM_NC (THREAD_SELF,
							  specific, idx1st);
      if (level2 == NULL)
	/* Not allocated, therefore no data.  */
	return NULL;

      /* There is data.  */
      data = &level2[idx2nd];
    }

  void *result = data->data;
  if (result != NULL)
    {
      uintptr_t seq = data->seq;

      if ( seq != ____pthread_keys[key].seq )
	result = data->data = NULL;
    }

  return result;
}

void *backend_get_tls(void)
{
    return ____pthread_getspecific(backend_key);
}

// from glibc
int ____pthread_setspecific (backend_key_t key, const void *value)
{
  struct backend *self;
  struct backend_key_data *level2;
  unsigned int idx1st;
  unsigned int idx2nd;
  unsigned int seq;

  self = THREAD_SELF;
  /* Special case access to the first 2nd-level block.  This is the usual case.  */
  if ( key < PTHREAD_KEY_2NDLEVEL_SIZE )
    {
      /* Verify the key is sane.  */
      if (KEY_UNUSED ((seq = ____pthread_keys[key].seq)))
	/* Not valid.  */
	return EINVAL;
      level2 = &self->specific_1stblock[key];
      /* Remember that we stored at least one set of data.  */
      if (value != NULL)
	THREAD_SETMEM (self, specific_used, 1);
    }
  else
    {
      if (key >= PTHREAD_KEYS_MAX
	  || KEY_UNUSED ((seq = ____pthread_keys[key].seq)))
	/* Not valid.  */
	return EINVAL;

      idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;
      idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;

      /* This is the second level array.  Allocate it if necessary.  */
      level2 = THREAD_GETMEM_NC (self, specific, idx1st);
      
      if (level2 == NULL)
	{
	  if (value == NULL)
	    /* We don't have to do anything.  The value would in any case
	       be NULL.  We can save the memory allocation.  */
	    return 0;

	  level2
	    = (struct backend_key_data *) calloc (PTHREAD_KEY_2NDLEVEL_SIZE,
						  sizeof (*level2));
	  if (level2 == NULL)
	    return ENOMEM;

	  THREAD_SETMEM_NC (self, specific, idx1st, level2);
	}

      /* Pointer to the right array element.  */
      level2 = &level2[idx2nd];

      /* Remember that we stored at least one set of data.  */
      THREAD_SETMEM (self, specific_used, 1);
    }

  /* Store the data and the sequence number so that we can recognize
     stale data.  */
  level2->seq = seq;
  level2->data = (void *) value;

  return 0;
}

void backend_set_tls(void *data)
{
    ____pthread_setspecific(backend_key, data);
}

void *backend_get_thread(void)
{
    return THREAD_SELF;
}

static int remote_init(void *dumm)
{
    return 0;
}

void backend_span_domain_default(int nos_threads)
{
    /* nop */
}

void backend_span_domain(int nos_threads, size_t stack_size)
{
    /* nop */
}

void backend_create_time(int cores)
{
    /* nop */
}

void backend_thread_exit(void)
{
}

struct thread *backend_thread_create_varstack(bomp_thread_func_t start_func,
                                              void *arg, size_t stacksize)
{
    start_func(arg);
    return NULL;
}
