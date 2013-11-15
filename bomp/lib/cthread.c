/*
 * cthread
 * clone based threading library (without futex)
 *
 * Copyright Antonio Barbalace, SSRG, VT, 2013
 */

// THIS LIBRARY IS WORK IN PROGRESS

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sched.h> //necessary for clone

#include <unistd.h>
#include <sys/syscall.h>
#include <asm/ldt.h>

#include "cthread.h"

/* the following code is extracted from glibc */
// /sysdeps/unix/sysv/linux/sched_setaffinty.c
#ifndef __NR_sched_setaffinity
  #error "syscall sched_setaffinity not defined"
#endif

#ifndef __NR_arch_prctl
  #error "syscall arch_prctl not defined"
#endif

#ifndef __NR_exit
  #error "syscall exit not defined"
#endif



// value copied from is.c
//NOTE that pthread glibc NPTL in gigi returns 8MB of stack per thread!
#define STACK_SIZE (8 * 1024 * 1024)

//#define DEBUG_MALLOC_BACKEND
#undef DEBUG_MALLOC_BACKEND
#ifdef DEBUG_MALLOC_BACKEND
     /* Prototypes for __malloc_hook, __free_hook */
     #include <malloc.h>

     /* Prototypes for our hooks.  */
     static void my_init_hook (void);
     static void *my_malloc_hook (size_t, const void *);
     //static void my_free_hook (void*, const void *);

     void* old_malloc_hook;

     /* Override initializing hook from the C library. */
     void (*__malloc_initialize_hook) (void) = my_init_hook;

     static void
     my_init_hook (void)
     {
       old_malloc_hook = __malloc_hook;
       //old_free_hook = __free_hook;
       __malloc_hook = my_malloc_hook;
       //__free_hook = my_free_hook;
     }

     static void *
     my_malloc_hook (size_t size, const void *caller)
     {
       void *result;
       unsigned long bp;
       /* Restore all old hooks */
       __malloc_hook = old_malloc_hook;
       //__free_hook = old_free_hook;
       /* Call recursively */
       result = malloc (size);
       /* Save underlying hooks */
       old_malloc_hook = __malloc_hook;
       //old_free_hook = __free_hook;
       /* printf might call malloc, so protect it too. */
       __asm__ __volatile__ ("movq %%rbp,%q0\n" : "=r" (bp));

       printf ("malloc (%u) returns %p bp %lx\n", (unsigned int) size, result, bp);
       /* Restore our own hooks */
       __malloc_hook = my_malloc_hook;
       //__free_hook = my_free_hook;
       return result;
     }
#endif

// to simulate pthread on exit
static inline unsigned long clone_exit (int ret)
{
  unsigned long _ret;
  __asm__ __volatile__ ("syscall\n"
			: "=a" (_ret)
			:"a" (__NR_exit), "d" (ret));
  return _ret;
}

#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003


#define __GET_FS(address) \
do { \
  syscall(__NR_arch_prctl, ARCH_GET_FS, &(address)); \
  printf("$fs id %u base 0x%lx _SELF %p ERRNO %p\n", \
	 TLS_GET_FS(), address, \
	 THREAD_SELF, __errno_location()); \
} while(0);

static inline unsigned long __set_fs (void * address)
{
    int _result;
    /* It is a simple syscall to set the %fs value for the thread.  */
    __asm__ __volatile__ ("syscall"
                    : "=a" (_result)
                    : "a" ((unsigned long int) __NR_arch_prctl),
                      "D" ((unsigned long int) ARCH_SET_FS),
                      "S" ((void*)address)
                    : "memory", "cc", "r11", "cx");
    return _result;
}

#undef offsetof
#define offsetof(S,F) ((size_t) & (((S *) 0)->F))







static size_t __kernel_cpumask_size = 0;
int cthread_setaffinity_np (pid_t pid, size_t cpusetsize, const cpu_set_t *cpuset)
{
    //return sched_setaffinity(pid,cpusetsize,cpuset);
  
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

      __kernel_cpumask_size = 8;
      printf("__kernel_cpumask_size is %d\n", psize);
  }

  // We now know the size of the kernel cpumask_t.
  // * Make sure the user does not request to set a bit beyond that.  
  size_t cnt;
  for (cnt = __kernel_cpumask_size; cnt < cpusetsize; ++cnt)
    if (((char *) cpuset)[cnt] != '\0')
      {
        // Found a nonzero byte.  This means the user request cannot be
	//fulfilled.  
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

typedef struct {
  int i[4];
} __128bits;

typedef enum {false, true} bool;
/* Type for the dtv.  */
typedef union dtv
{
  size_t counter;
  struct
  {
    void *val;
    bool is_static;
  } pointer;
} dtv_t;
// todo allocate_dtv from /gnu/glibc/elf/dl-tls.c

typedef struct
{
  void *tcb;		/* Pointer to the TCB.  Not necessarily the
			   thread descriptor used by libpthread.  */
  dtv_t *dtv;
  void *self;		/* Pointer to the thread descriptor.  */
  int multiple_threads;
  int gscope_flag;
  uintptr_t sysinfo;
  uintptr_t stack_guard;
  uintptr_t pointer_guard;
  unsigned long int vgetcpu_cache[2];
#undef __ASSUME_PRIVATE_FUTEX
# ifndef __ASSUME_PRIVATE_FUTEX
  int private_futex;
# else
  int __unused1;
# endif
  int rtld_must_xmm_save;
  /* Reservation of some values for the TM ABI.  */
  void *__private_tm[4];
  /* GCC split stack support.  */
  void *__private_ss;
  long int __unused2;
  /* Have space for the post-AVX register size.  */
  __128bits rtld_savespace_sse[8][4] __attribute__ ((aligned (32)));

  void *__padding[8];
} tcbhead_t;

//from glibc/nptl/descr.h
//in struct pthread
struct backend {
  tcbhead_t header; // header is the first field in any case
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
	  : "i" (offsetof (struct backend, header.self)));	 	      \
     __self;})

cthread_t cthread_self (void )
{	return (cthread_t)THREAD_SELF; }





/* Macros to load from and store into segment registers.  */
# define TLS_GET_FS() \
  ({ int __seg; __asm ("movl %%fs, %0" : "=q" (__seg)); __seg; })
# define TLS_SET_FS(val) \
   __asm ("movl %0, %%fs" :: "q" (val))

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

      /* Read member of the thread descriptor directly.  */
# define THREAD_GETMEM(descr, member) \
  ({ __typeof (descr->member) __value;					      \
     if (sizeof (__value) == 1)						      \
       asm volatile ("movb %%fs:%P2,%b0"				      \
		     : "=q" (__value)					      \
		     : "0" (0), "i" (offsetof (struct backend, member)));     \
     else if (sizeof (__value) == 4)					      \
       asm volatile ("movl %%fs:%P1,%0"					      \
		     : "=r" (__value)					      \
		     : "i" (offsetof (struct backend, member)));	      \
     else								      \
       {								      \
	 if (sizeof (__value) != 8)					      \
	   /* There should not be any value with a size other than 1,	      \
	      4 or 8.  */						      \
	   abort ();							      \
									      \
	 asm volatile ("movq %%fs:%P1,%q0"				      \
		       : "=r" (__value)					      \
		       : "i" (offsetof (struct backend, member)));	      \
       }								      \
     __value; })

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

# define THREAD_COPY_STACK_GUARD(descr) \
    ((descr)->header.stack_guard					      \
     = THREAD_GETMEM (THREAD_SELF, header.stack_guard))

# define THREAD_COPY_POINTER_GUARD(descr) \
  ((descr)->header.pointer_guard					      \
   = THREAD_GETMEM (THREAD_SELF, header.pointer_guard))



/* Table of the key information.  */
static struct pthread_key_struct ____pthread_keys[PTHREAD_KEYS_MAX];

int cthread_key_create (cthread_key_t *key, void (*destr) (void *))
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

#ifdef __GNUC__
# define roundup(x, y)  (__builtin_constant_p (y) && powerof2 (y)             \
                         ? (((x) + (y) - 1) & ~((y) - 1))                     \
                         : ((((x) + ((y) - 1)) / (y)) * (y)))
#else
# define roundup(x, y)  ((((x) + ((y) - 1)) / (y)) * (y))
#endif
#define powerof2(x)     ((((x) - 1) & (x)) == 0)

/* Size of the static TLS block.  Giving this initialized value
   preallocates some surplus bytes in the static TLS area.  */
#define SMALL_TLS 
#ifdef SMALL_TLS
size_t _dl_tls_static_size = 1024;
#else
size_t _dl_tls_static_size = 2048;
#endif

# define TLS_INIT_TCB_ALIGN __alignof__ (struct backend)

/* Number of additional slots in the dtv allocated.  */
#define DTV_SURPLUS (14)
static int dl_tls_max_dtv_idx = 1;
// from glibc/elf/dl-tls.c
// NOTE result is the pointer to tcbhead_t*
static void* allocate_dtv (void *result)
{
  dtv_t *dtv;
  size_t dtv_length;

  /* We allocate a few more elements in the dtv than are needed for the
     initial set of modules.  This should avoid in most cases expansions
     of the dtv.  */
  dtv_length = dl_tls_max_dtv_idx + DTV_SURPLUS;
  dtv = calloc (dtv_length + 2, sizeof (dtv_t)); // Antonio: must use calloc to work with deallocate_dtv
  memset (dtv, 0, ((dtv_length +2)*sizeof(dtv_t))); // added by Antonio
  if (dtv != NULL)
    {
      /* This is the initial length of the dtv.  */
      dtv[0].counter = dtv_length;

      /* The rest of the dtv (including the generation counter) is
	 Initialize with zero to indicate nothing there.  */

      /* Add the dtv to the thread data structures.  */
/* Install the dtv pointer.  The pointer passed is to the element with
   index -1 which contain the length.  */
  ((tcbhead_t *) (result))->dtv = (dtv) + 1;
    }
  else
    result = NULL;

  return result;
}

extern void * _dl_allocate_tls_init (void *result); // accessible trought the ld library loader

#define DUMP_CUR_TCB
#ifdef DUMP_CUR_TCB
void dump_current_tcb()
{
    printf("tcbhead .tcb %p .dtv %p .self %p .multiple_threads %d .gscope %d\n",
	 THREAD_GETMEM(THREAD_SELF, header.tcb),
	 THREAD_GETMEM(THREAD_SELF, header.dtv),
	 THREAD_GETMEM(THREAD_SELF, header.self),
	 THREAD_GETMEM(THREAD_SELF, header.multiple_threads),
	 THREAD_GETMEM(THREAD_SELF, header.gscope_flag));
    printf("tcbhead .sysinfo %lx .stack_guard %lx .ptr_guard %lx .vgetcpu_ %lx:%lx\n",
	 THREAD_GETMEM(THREAD_SELF, header.sysinfo),
	 THREAD_GETMEM(THREAD_SELF, header.stack_guard),
	 THREAD_GETMEM(THREAD_SELF, header.pointer_guard),
	 THREAD_GETMEM(THREAD_SELF, header.vgetcpu_cache[0]),
	 THREAD_GETMEM(THREAD_SELF, header.vgetcpu_cache[1]));
    printf("tcbhead .futex %d .rtld_must_xmm_save %d .private_ss %p\n",
# ifndef __ASSUME_PRIVATE_FUTEX
	  THREAD_GETMEM(THREAD_SELF, header.private_futex),
# else
	  THREAD_GETMEM(THREAD_SELF, header.__unused1),
# endif
	 THREAD_GETMEM(THREAD_SELF, header.rtld_must_xmm_save),
	 THREAD_GETMEM(THREAD_SELF, header.__private_ss));
}
#else
#define dump_current_tcb()
#endif

#define DUMP_CUR_DTV
#ifdef DUMP_CUR_DTV
void dump_current_dtv()
{
    int i, count, max;
    dtv_t * dtva;
    printf("%s\n",__func__);
    dtva = THREAD_GETMEM(THREAD_SELF, header.dtv);

  /* NOTE the following I am not sure will work, must be checked : maybe must accessed via FS
   * NOTE DTV id(-1) has the number of elements available in the array,
   * NOTE DTV id(0) the current number of elements in the array
   */
    max = dtva[-1].counter;
    count = dtva[0].counter;

    for (i=-1; (i<(count +1) && (i<max)); i++)
	printf("dtv[%2d] {counter:%ld} U {val:%p is_static:%x}\n",
	      i, dtva[i].counter, dtva[i].pointer.val, dtva[i].pointer.is_static);
    printf("~%s\n",__func__);
}
#else
#define dump_current_dtv()
#endif



//static unsigned long saved_selector = -1; // now in popcorn_backend.c
static unsigned long selector = -1;

// from glibc
void * cthread_getspecific (cthread_key_t key)
{
  struct backend_key_data *data;

    if (key == -1)
      printf("%s: ERROR key == -1\n", __func__);

  /* Special case access to the first 2nd-level block.  This is the
     usual case.  */
  if ( key < PTHREAD_KEY_2NDLEVEL_SIZE ) {
    data = &(THREAD_SELF->specific_1stblock[key]);
  }else
    {
      /* Verify the key is sane.  */
      if (key >= PTHREAD_KEYS_MAX) {
	/* Not valid.  */
	printf("%s: ERROR key >= PTHREAD_KEYS_MAX (%d)\n", __func__, key);
	return NULL;
      }

      unsigned int idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;
      unsigned int idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;

      /* If the sequence number doesn't match or the key cannot be defined
	 for this thread since the second level array is not allocated
	 return NULL, too.  */
      struct backend_key_data *level2 = THREAD_GETMEM_NC (THREAD_SELF,
							  specific, idx1st);
      if (level2 == NULL) {
	/* Not allocated, therefore no data.  */
	printf("%s: ERROR level2 == NULL\n", __func__);
	return NULL;
      }

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

//printf("get_specific %d(%ld) %p [%p]\n", key, data->seq, result, THREAD_SELF); sleep(1);
  return result;
}

// from glibc
int cthread_setspecific (cthread_key_t key, const void *value)
{
  struct backend *self;
  struct backend_key_data *level2;
  unsigned int idx1st;
  unsigned int idx2nd;
  unsigned int seq;

  if (key == -1)
    printf("%s: ERROR key == -1\n", __func__);

  self = THREAD_SELF;

  /* Special case access to the first 2nd-level block.  This is the usual case.  */
  if ( key < PTHREAD_KEY_2NDLEVEL_SIZE )
    {
      /* Verify the key is sane.  */
      if (KEY_UNUSED ((seq = ____pthread_keys[key].seq))) {
	/* Not valid.  */
	printf("%s: ERROR KEY_UNUSED key %d seq %d\n", __func__, key, seq);
	return EINVAL;
      }
      level2 = &(self->specific_1stblock[key]);
      /* Remember that we stored at least one set of data.  */
      if (value != NULL)
	THREAD_SETMEM (self, specific_used, 1);
    }
  else
    {
      if (key >= PTHREAD_KEYS_MAX
	  || KEY_UNUSED ((seq = ____pthread_keys[key].seq))) {
	/* Not valid.  */
	printf("%s: ERROR key >= PTHREAD_KEYS_MAX (%d)\n", __func__, key);
	return EINVAL;
      }

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

//printf("set_specific %u(%ld) %p [%p]\n", key, level2->seq, level2->data, self); sleep(1);
  return 0;
}


/*
 * this routine switch from pthread to cthread
 */

//TODO can fail?!
//TODO do I really need the the backend pointer (no because is saved in TLS)
unsigned long cthread_initialize ()
{
	unsigned long saved_selector;

	// TLS_TCB ALLOCATION ---------------------------------------------------------
	//from GLIBC /gnu/glibc/csu/libc-tls.c __libc_setup_tls
	  int memsz = 0; //memsz = phdr->p_memsz; <-- if there is a segment with phdr
	  int tcb_offset= memsz + roundup(_dl_tls_static_size, 1);
	  struct backend * __backendptr =
	    malloc( tcb_offset + sizeof(struct backend) + TLS_INIT_TCB_ALIGN);
	  if (!__backendptr) {
	      printf("%s: TLS allocation error\n", __func__);
	      exit (0);
	  }
	  memset(__backendptr, 0, sizeof(struct backend));

	  struct backend * backendptr = (void *) (((char*)__backendptr) + tcb_offset);

	  printf("sizeof(struct backend) %ld(0x%lx) ",
		 sizeof(struct backend), sizeof(struct backend));
	  printf("__backend %p backend %p (last addr %p)\n",
		 __backendptr, backendptr, ((char*) backendptr) + sizeof (struct backend));

	// get the previous FS --------------------------------------------------------
	  __GET_FS(saved_selector);

	// COPY TCB -------------------------------------------------------------------
	    long i;
	    for (i = 0; i < (signed int)sizeof( tcbhead_t); i++)
	    {
		unsigned char __value;
		asm volatile ("movb %%fs:%P2(%q3),%b0"
			      : "=q" (__value) : "0" (0), "i" (0), "r" (i));
		((unsigned char *)backendptr)[i] = __value;
	    }
	#define DUMP_BACKEND
	#ifdef DUMP_BACKEND
	//printf("\n");
	printf("backendptr->header.self %p backendptr->header.tcb %p self %p\n",
		backendptr->header.self, backendptr->header.tcb, THREAD_SELF);
	#endif

	// INSTALL TCB ----------------------------------------------------------------
	    backendptr->header.self = backendptr; // <--- from pthread_create.c
	    backendptr->header.tcb = backendptr; // <--- from pthread_create.c

	    backendptr->specific[0] = backendptr->specific_1stblock;
	    backendptr->header.multiple_threads = 1; // ready to switch multithread - this is an heuristic

	// DTV ALLOCATION -------------------------------------------------------------
	    void* retptr = allocate_dtv(backendptr);
	    if (retptr == NULL) {
	      printf("allocation of dtv failed for cpu MAIN, exit\n");
	      exit(0);
	    }
	    printf("retptr %p (backendptr)\n", retptr);

	// INSTALL DTV ----------------------------------------------------------------
	    if (_dl_allocate_tls_init (backendptr) == NULL) {
	      printf("allocation of tls failed for cpu MAIN, exit\n");
	      exit(0);
	    }
	  dump_current_tcb();
	  dump_current_dtv();
        printf("copying previous tls\n");
	// COPY previous TLS -----------------------------------------------------------
	    for (i = -tcb_offset; i < 0; i++)
	    {
		unsigned char __value;
		asm volatile ("movb %%fs:%P2(%q3),%b0"
			      : "=q" (__value) : "0" (0), "i" (0), "r" (i));
		((unsigned char *)backendptr)[i] = __value;
	    }
        printf("done copying previous tls\n");
        printf("setting fs and getting fs\n");
	// INSTALL NEW TCB ------------------------------------------------------------
	    __set_fs(backendptr);
	    __GET_FS(selector);
        printf("done getting fs\n");

	//printf("ERRNO %p\n", __errno_location ());
	//printf("sleeping\n"); sleep(1);
    printf("dumping tls tcb\n");
	#ifdef DUMP_TLS_TCB
	    for (i = -tcb_offset ; i < (signed int)sizeof( tcbhead_t); i++) {
		unsigned char __value;
		asm volatile ("movb %%fs:%P2(%q3),%b0"
			     : "=q" (__value) : "0" (0), "i" (0), "r" (i));
	    if (!i) printf("\n"); //separate TLS from TCB
	    printf("%.2x ", (unsigned int)__value);
	    }
	#endif
    printf("done dumping tls tcb\n");

    printf("dumping backend\n");
	#ifdef DUMP_BACKEND
	//printf("sleeping\n"); sleep(1);
	printf("backendptr->header.self %p backendptr->header.tcb %p self %p\n",
		backendptr->header.self, backendptr->header.tcb, THREAD_SELF);
	#endif
    printf("done dumping backend\n");

	  dump_current_tcb();
	  dump_current_dtv();

  return saved_selector;
}

void cthread_restore (unsigned long selector)
{

	// TODO
	//here there is maybe memory to liberate before switching context

	{
	    int _result;
	/* It is a simple syscall to set the %fs value for the thread.  */
	      __asm__ __volatile__ ("syscall"
	                    : "=a" (_result)
	                    : "a" ((unsigned long int) __NR_arch_prctl),
	                      "D" ((unsigned long int) ARCH_SET_FS),
	                      "S" ((void*)selector)
	                    : "memory", "cc", "r11", "cx");
	_result;
	}

}




//TODO MUST BE INTEGRATEEEE!!!
// the following must be integrated in struct backend
typedef struct _backend_args {
  void * user;
  int (* cfunc)(void * args);
} backend_args;

int backend_start_func (void * args)
{
  int ret =
    ((backend_args *)args)->cfunc(((backend_args *) args)->user);



/*  printf("%s: barg %p carg %p cfunc %p\n",
    __func__,  args, ((backend_args *)args)->cfunc,((backend_args *) args)->user);
    */ // for some reason (concurrency..) printf is using futex.. so avoid it in the current environment
//  free( args );

  // if to maintain here the following REMEMBER TO ADD DEFINES
/*   dump_current_tcb();
  dump_current_dtv();
 */


  clone_exit(ret);
  /* never reaching here */
  return -1;
}

// we assume stack is growing downward (tipical in x86)
//#define STACK_SIZE 0x1000 //have a look at the beginning of the program!
// from glibc
#define CLONE_SIGNAL            (CLONE_SIGHAND | CLONE_THREAD)

//int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
int cthread_create (cthread_t *thread, void* attr, void *(*cfunc)(void*), void *arg)
{
	int core_id = (int) attr;
  int r = 0; // pthread_create(&pthread, NULL, cfunc, arg);
	#define GLIBC_STYLE 0
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
	   //int clone_flags = (CLONE_THREAD|CLONE_SIGHAND|CLONE_VM); // Marina's version
	    int clone_flags = (CLONE_VM | CLONE_SIGNAL
			     | CLONE_SETTLS | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID
			     | 0);

	#endif
	    pid_t ptid, ctid;

    printf("HELLO?");
// TODO liberate the memory before creating new threads
// TODO --- we can maintain a list ---
// TODO

	    void * stack = malloc(STACK_SIZE);
	    if (!stack) {
		printf("stack allocation error");
		exit (0);
	    }
	    memset(stack, 0, STACK_SIZE);

	    int memsz = 0; //memsz = phdr->p_memsz; <-- if there is a segment with phdr
	    int tcb_offset = memsz + roundup(_dl_tls_static_size, 1);
	    struct backend * __backendptr = malloc( tcb_offset + sizeof(struct backend) + TLS_INIT_TCB_ALIGN) ;
	    if (!__backendptr) {
		printf("%s: TLS allocation error\n", __func__);
		exit (0);
	    }
	    memset(__backendptr, 0, ( tcb_offset + sizeof(struct backend) + TLS_INIT_TCB_ALIGN));
	#ifdef DEBUG_MALLOC_BACKEND
	    printf("malloc(tcb_offset %d, struct backend %ld, TLS_INIT_TCB %ld, total %ld) @ %p\n",
		  tcb_offset, sizeof(struct backend), TLS_INIT_TCB_ALIGN,
		  (tcb_offset + sizeof(struct backend) + TLS_INIT_TCB_ALIGN), __backendptr);
	#endif

	    struct backend * backendptr = (void *) ((char*)__backendptr + tcb_offset);
	    backendptr->header.self = backendptr; // <--- from pthread_create.c:501
	    backendptr->header.tcb = backendptr; // <--- from pthread_create.c:504
	    // nptl/allocatestack.c
	      /* The first TSD block is included in the TCB.  */
	    backendptr->specific[0] = backendptr->specific_1stblock;
	    backendptr->header.multiple_threads = 1;

	//extern int * __libc_multiple_threads_ptr;
	//    *__libc_multiple_threads_ptr = 1;

	    THREAD_COPY_STACK_GUARD (backendptr);
	    THREAD_COPY_POINTER_GUARD (backendptr);

	#ifndef __ASSUME_PRIVATE_FUTEX
	      /* The thread must know when private futexes are supported.  */
	      backendptr->header.private_futex =
		  THREAD_GETMEM (THREAD_SELF, header.private_futex);
	#endif
	    /* Copy the sysinfo value from the parent.  */
	    backendptr->header.sysinfo = THREAD_GETMEM (THREAD_SELF, header.sysinfo);

	    // DTV support /gnu/glibc/elf/dl-tls.c
	    void* retptr = allocate_dtv(backendptr);
	    if (retptr == NULL) {
	      printf("allocation of dtv failed for cpu %d, exit\n", core_id);
	      exit(0);
	    }
	    if (_dl_allocate_tls_init (retptr) == NULL) {
	      printf("allocation of tls failed for cpu %d, exit\n", core_id);
	      exit(0);
	    }

	    backend_args * bkargs = malloc(sizeof(backend_args));
	    if (!bkargs) {
		printf("args allocation error\n");
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
	    //r = clone(backend_start_func, (stack+STACK_SIZE), clone_flags, bkargs, &ptid, 0, &ctid);
	    r = clone(backend_start_func, (stack+STACK_SIZE), clone_flags, bkargs, &ptid, backendptr, &ctid);
	    if (r == -1) {
	        //printf("clone failed\n");
		perror("clone failed");
//		exit (0);
	    }
	    printf ("%s: TID %d, cpuid %d stack %p tls %p barg %p carg %p bfunc %p cfunc %p\n",
		    __func__, r, core_id,
		    (stack+STACK_SIZE), backendptr,
		    bkargs, arg, backend_start_func, cfunc);


// TODO	    DO I HAVE TO SAVE _backend_args here?!

    printf("Goodbye\n");
return r;

}



