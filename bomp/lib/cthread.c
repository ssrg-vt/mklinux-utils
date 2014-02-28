/*
 * cthread
 * clone based threading library (without futex)
 * it implements the model TLS_TCB_AT_TP ONLY
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
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <asm/ldt.h>
//#include <sys/types.h> //gettid

#include "cthread.h"

/* the following code is extracted from glibc */
// /sysdeps/unix/sysv/linux/sched_setaffinty.c
#ifndef __NR_sched_setaffinity
  #error "syscall sched_setaffinity not defined"
#endif
#ifndef __NR_sched_getaffinity
  #error "syscall sched_getaffinity not defined"
#endif

#ifndef __NR_arch_prctl
  #error "syscall arch_prctl not defined"
#endif

#ifndef __NR_exit
  #error "syscall exit not defined"
#endif

#ifndef __NR_gettid
  #error "syscall gettid not defined"
//  #define __NR_gettid   186
#endif

#define GET_TID ((unsigned long)syscall(__NR_gettid))

#ifdef DEBUG_MALLOC_BACKEND
// the following code has been copied from a webpage 
/* Prototypes for __malloc_hook, __free_hook */
#include <malloc.h>

/* Prototypes for our hooks.  */
static void my_init_hook (void);
static void *my_malloc_hook (size_t, const void *);
//static void my_free_hook (void*, const void *);

void* old_malloc_hook;

/* Override initializing hook from the C library. */
void (*__malloc_initialize_hook) (void) = my_init_hook;

static void my_init_hook (void)
     {
       old_malloc_hook = __malloc_hook;
       //old_free_hook = __free_hook;
       __malloc_hook = my_malloc_hook;
       //__free_hook = my_free_hook;
     }

static void * my_malloc_hook (size_t size, const void *caller)
     {
       void *result;
//       unsigned long bp;
       /* Restore all old hooks */
       __malloc_hook = old_malloc_hook;
       //__free_hook = old_free_hook;
       /* Call recursively */
       result = malloc (size);
       /* Save underlying hooks */
       old_malloc_hook = __malloc_hook;
       //old_free_hook = __free_hook;
       /* printf might call malloc, so protect it too. */
//       __asm__ __volatile__ ("movq %%rbp,%q0\n" : "=r" (bp));

       printf ("%ld: malloc(%u) (%p-%p) \n",   // bp %lx\n",
	        GET_TID, (unsigned int) size,
	       result, result + size); //PREVIOUS VERSION bp);
       /* Restore our own hooks */
       __malloc_hook = my_malloc_hook;
       //__free_hook = my_free_hook;
       return result;
     }
#endif

// to simulate pthread on exit (the original code from glibc/nptl/sysdeps/x86_64/pthreaddef.h follows)
/* While there is no such syscall.  */
//#define __exit_thread_inline(val) \
//  asm volatile ("syscall" :: "a" (__NR_exit), "D" (val))
static inline unsigned long clone_exit (int ret)
{
  unsigned long _ret;
  __asm__ __volatile__ ("syscall\n"
			: "=a" (_ret)
			:"a" (__NR_exit), "D" (ret)
			:"memory");
  return _ret;
}

#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003

/*
#define __GET_FS(address) \
do { \
  syscall(__NR_arch_prctl, ARCH_GET_FS, &(address)); \
  printf("$fs id %u base 0x%lx _SELF %p ERRNO %p\n", \
	 TLS_GET_FS(), address, \
	 THREAD_SELF, __errno_location()); \
} while(0);
*/

#define __GET_FS(address) \
  syscall(__NR_arch_prctl, ARCH_GET_FS, &(address)); \

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

#ifdef REDEFINE_SETAFFINITY
static size_t __kernel_cpumask_size = 8;//it was 0
int cthread_setaffinity_np (pid_t pid, size_t cpusetsize, const cpu_set_t *cpuset)
{
  if (__kernel_cpumask_size == 0 )
  {
    int res;
    size_t psize = 8;
    void *p = malloc (psize);
    memset(p, 0xFF, psize);

    while ( (res = syscall (__NR_sched_getaffinity, pid, psize, p)) < 0 ) {
      psize = 2*psize;
      p = (void*) realloc (p, psize);
      memset(p, 0xFF, psize);
//      printf("try %d %d\n", psize, res);
    }

      __kernel_cpumask_size = psize;
//      printf("__kernel_cpumask_size is %d\n", psize);
      free(p);
  }

  /* We now know the size of the kernel cpumask_t.
   * Make sure the user does not request to set a bit beyond that.  */
  size_t cnt;
  for (cnt = __kernel_cpumask_size; cnt < cpusetsize; ++cnt)
    if (((char *) cpuset)[cnt] != '\0')
      {
        /* Found a nonzero byte.  This means the user request cannot be
	fulfilled.  */
	return 4321;
      }

if (cpusetsize > __kernel_cpumask_size)
  cpusetsize = __kernel_cpumask_size;
#ifdef USE_SYSCALL_SETAFFINITY
  int result = syscall (__NR_sched_setaffinity, pid, cpusetsize, cpuset);
#else  
  unsigned long result = 0;
  __asm__ __volatile__ ("syscall\n"
			: "=a" (result)
			: "a" (__NR_sched_setaffinity), "D" (pid), "S" (cpusetsize), "d" (cpuset)
			: "memory");
#endif
  return result;
}
#else
int cthread_setaffinity_np (pid_t pid, size_t cpusetsize, const cpu_set_t *cpuset)
{
    return sched_setaffinity(pid,cpusetsize,cpuset);
}
#endif

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
  
  /* Thread ID */
  pid_t tid;
  /* Process ID - thread group ID in kernel speak.  */
  pid_t pid;
  
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
  
  /* True if the user provided the stack.  */
//  bool user_stack;
  
    /* The result of the thread function.  */
  void *result;

  /* Start position of the code to be executed and the argument passed
     to the function.  */
  void *(*start_routine) (void *);
  void *arg;

  /* If nonzero pointer to area allocated for the stack and its
     size.  */
  void *stackblock;
  size_t stackblock_size;
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
#ifdef SMALL_TLS
static size_t _dl_tls_static_size = 1024;
#else
static size_t _dl_tls_static_size = 2048;// + 2400;
#endif
//extern size_t _dl_tls_static_size; // NOTE in case of static compilation with glibc this is a global var, i.e. this is a smart way to get this informations in order for cthread to work correctly

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
//  memset (dtv, 0, ((dtv_length +2)*sizeof(dtv_t))); // added by Antonio --- calloc already provides zeroed memory
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

#ifdef DUMP_CUR_DTV
void dump_current_dtv()
{
    int i, count, max;
    dtv_t * dtva;
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

extern const uint16_t ** __attribute__ ((const)) __ctype_b_loc (void);
extern const int32_t ** __attribute__ ((const)) __ctype_toupper_loc (void);
extern const int32_t ** __attribute__ ((const)) __ctype_tolower_loc (void);

static const uint16_t * vctype_b = 0;
static const int32_t * vctype_toupper = 0;
static const int32_t * vctype_tolower = 0;

extern __thread void * __resp;
static const void * v__resp;

/*
 * this routine switch from pthread to cthread
 */

//TODO can fail?!
//TODO do I really need the the backend pointer (no because is saved in TLS)
unsigned long __cthread_initialize ()
{
	unsigned long saved_selector;
	
// in glibc/nptl-init.c:280 __pthread_initialize_minimal_internal() they are doing:
//  struct pthread *pd = THREAD_SELF;
// because this is for dynamically linked library we skipped the code in #ifndef SHARED i.e. static compiled
	
	// TLS_TCB ALLOCATION ---------------------------------------------------------
	//from GLIBC /gnu/glibc/csu/libc-tls.c __libc_setup_tls
	  int memsz = 0; //memsz = phdr->p_memsz; <-- if there is a segment with phdr
	  int tcb_offset= memsz + roundup(_dl_tls_static_size, 1);
	  int _backend_size = tcb_offset + sizeof(struct backend) + TLS_INIT_TCB_ALIGN;
	  struct backend * __backendptr = calloc(_backend_size, 1);
	  if (!__backendptr) {
	      printf("%s: TLS allocation error\n", __func__);
	      exit (0);
	  }
	  //memset(__backendptr, 0, sizeof(struct backend)); //removed after using calloc

	  struct backend * backendptr = (void *) (((char*)__backendptr) + tcb_offset);

#ifdef DUMP_BACKEND	  
	  printf("sizeof(struct backend) %ld(0x%lx) ",
		 sizeof(struct backend), sizeof(struct backend));
	  printf("__backend %p backend %p (last addr %p)\n",
		 __backendptr, backendptr, ((char*) backendptr) + sizeof (struct backend));
#endif
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
#ifdef DUMP_BACKEND	    
	    printf("retptr %p (backendptr)\n", retptr);
#endif	    

	// INSTALL DTV ----------------------------------------------------------------
	    if (_dl_allocate_tls_init (backendptr) == NULL) {
	      printf("allocation of tls failed for cpu MAIN, exit\n");
	      exit(0);
	    }
	  dump_current_tcb();
	  dump_current_dtv();

	  const uint16_t ** pctype_b = __ctype_b_loc ();
//	  const unsigned long offctype_b = (unsigned long)saved_selector - (unsigned long)pctype_b;
	  const int32_t ** pctype_toupper = __ctype_toupper_loc ();
//	  const unsigned long offctype_toupper = (unsigned long)saved_selector - (unsigned long)pctype_toupper;
	  const int32_t ** pctype_tolower = __ctype_tolower_loc ();
//	  const unsigned long offctype_tolower = (unsigned long)saved_selector - (unsigned long)pctype_tolower;    
	    
	  vctype_b = (pctype_b) ? *pctype_b : 0;
	  vctype_toupper = (pctype_toupper) ? *pctype_toupper : 0;
	  vctype_tolower = (pctype_tolower) ? *pctype_tolower : 0;	  

	  v__resp = __resp;
#ifdef DEBUG_TLS
  printf("ctype_b %p{%p} ctype_toupper %p{%p} ctype_tolower %p{%p} ", 
	pctype_b, *pctype_b,
	pctype_toupper, *pctype_toupper, pctype_tolower, *pctype_tolower); 
  printf("errno %p{%d} __resp %p{%p}\n",
	 __errno_location(), *(int *)__errno_location(), &__resp, __resp);
#endif 	  
	
#ifdef MEMCPY_TLS  
	    // COPY previous TLS ------------------------------------------------------
	    for (i = -tcb_offset; i < 0; i++)
	    {
		unsigned char __value;
		asm volatile ("movb %%fs:%P2(%q3),%b0"
			      : "=q" (__value) : "0" (0), "i" (0), "r" ((long)i));
		((unsigned char *)backendptr)[i] = __value;
	    }
#endif
	// INSTALL NEW TCB ------------------------------------------------------------
	    __set_fs(backendptr);
	    __GET_FS(selector);
	    
#ifndef MEMCPY_TLS	    
	// RESTORE TLS (alternative to COPY previos TLS -------------------------------
	    { // copyied from bakend_start_fn
  __resp = (void*) v__resp; //it seems that this was already initialized (default value?!)
  
  const uint16_t ** pctype_b = __ctype_b_loc ();
  const int32_t ** pctype_toupper = __ctype_toupper_loc ();
  const int32_t ** pctype_tolower = __ctype_tolower_loc ();

  if (pctype_b) *pctype_b = vctype_b;
  if (pctype_toupper) *pctype_toupper = vctype_toupper;
  if (pctype_tolower) *pctype_tolower = vctype_tolower;
	    }
#endif
	    
#ifdef DEBUG_TLS
  printf("ctype_b %p{%p} ctype_toupper %p{%p} ctype_tolower %p{%p} ", 
	pctype_b, *pctype_b,
	pctype_toupper, *pctype_toupper, pctype_tolower, *pctype_tolower); 
  printf("errno %p{%d} __resp %p{%p}\n",
	 __errno_location(), *(int *)__errno_location(), &__resp, __resp);
#endif 	  
// this two are alternative  
	#ifdef DUMP_TLS_TCB
	    for (i = -tcb_offset ; i < (signed int)sizeof( tcbhead_t); i++) {
		unsigned char __value;
		asm volatile ("movb %%fs:%P2(%q3),%b0"
			     : "=q" (__value) : "0" (0), "i" (0), "r" ((long)i));
	    if (!i) printf("\n\n"); //separate TLS from TCB
	    printf("%.2x ", (unsigned int)__value);
	    }
	#endif

/*	#ifdef DUMP_BACKEND
	printf("backendptr->header.self %p backendptr->header.tcb %p self %p\n",
		backendptr->header.self, backendptr->header.tcb, THREAD_SELF);
	#endif
*/	  dump_current_tcb();
	  dump_current_dtv();

  return saved_selector;
}

static size_t __static_tls_align_m1, __static_tls_size;
#define STACK_ALIGN 16
#define ARCH_STACK_DEFAULT_SIZE (2 * 1024 * 1024)
#define PTHREAD_STACK_MIN 16384
#define MINIMAL_REST_STACK 2048 

static size_t  __default_cthread_attr_stacksize;
static size_t  __default_cthread_attr_guardsize;
extern void * __libc_stack_end;

//copied from nptl/nptl-init.c:280 __pthread_initialize_minimal_internal
unsigned long cthread_initialize_minimal (void)
{
  struct backend * backendptr = THREAD_SELF;
  backendptr->pid = backendptr->tid = GET_TID;
  backendptr->specific[0] = &backendptr->specific_1stblock[0];
//  THREAD_SETMEM (pd, user_stack, true); // the operating system is providing the stack - not used
  backendptr->stackblock_size = (size_t) __libc_stack_end; // doesn't really have sense but ..

// TLS BLOCK ------------------------------------------------------------------
  /* Get the size of the static and alignment requirements for the TLS block.  */
  size_t static_tls_align;
  _dl_get_tls_static_info (&__static_tls_size, &static_tls_align);

  /* Make sure the size takes all the alignments into account.  */
  if (STACK_ALIGN > static_tls_align)
    static_tls_align = STACK_ALIGN;
  __static_tls_align_m1 = static_tls_align - 1;

  __static_tls_size = roundup (__static_tls_size, static_tls_align);
  printf("__static_tls_size 0x%lx __static_tls_align_m1 0x%lx\n",
	 __static_tls_size, __static_tls_align_m1);

// STACK size and alignment ---------------------------------------------------  
  // Determine the default allowed stack size.  This is the size used in case the user does not specify one.
  struct rlimit limit;
  if (getrlimit (RLIMIT_STACK, &limit) != 0 || limit.rlim_cur == RLIM_INFINITY)
    /* The system limit is not usable.  Use an architecture-specific default. */
    limit.rlim_cur = ARCH_STACK_DEFAULT_SIZE;
  else if (limit.rlim_cur < PTHREAD_STACK_MIN)
    /* The system limit is unusably small.
       Use the minimal size acceptable.  */
    limit.rlim_cur = PTHREAD_STACK_MIN;

  /* Make sure it meets the minimum size that allocate_stack
     (allocatestack.c) will demand, which depends on the page size.  */
  const uintptr_t pagesz = 4096;
  const size_t minstack = pagesz + __static_tls_size + MINIMAL_REST_STACK;
  if (limit.rlim_cur < minstack)
    limit.rlim_cur = minstack;

  /* Round the resource limit up to page size.  */
  limit.rlim_cur = (limit.rlim_cur + pagesz - 1) & -pagesz;
  __default_cthread_attr_stacksize = limit.rlim_cur;
  __default_cthread_attr_guardsize = pagesz;
  
  printf("__..stacksize %ld __..guardsize 0x%lx\n",
    (unsigned long)__default_cthread_attr_stacksize, (unsigned long)__default_cthread_attr_guardsize);

// TODO ?!
  /* Transfer the old value from the dynamic linker's internal location.  */
//  *__libc_dl_error_tsd () = *(*GL(dl_error_catch_tsd)) ();
//  GL(dl_error_catch_tsd) = &__libc_dl_error_tsd;
  
// NOTE NOTE NOTE here there are a lot of function call registrations  
  
// from /gnu/glibc/nptl/sysdeps/unix/sysv/linux/libc_pthread_init.c  
//  __libc_pthread_init (&__fork_generation, __reclaim_stacks,ptr_pthread_functions); // WE ARE NOT SUPPORTING FORK

  /* Determine whether the machine is SMP or not.  */
  //__is_smp = is_smp_system ();
  return 0;
}

unsigned long cthread_initialize(void)
{
#ifdef INIT_MINIMAL
  return cthread_initialize_minimal();
#else
  return __cthread_initialize();
#endif
}

void __cthread_restore (unsigned long selector)
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

void cthread_restore (unsigned long selector)
{
#ifdef INIT_MINIMAL
  __cthread_restore(selector);
#endif
}

//TODO MUST BE INTEGRATEEEE!!!
// the following must be integrated in struct backend
typedef struct _backend_args {
  void * user;
  void * stackblock;
  void * (* cfunc)(void * args);
} backend_args;

int backend_start_func (void * args)
{
  int _ret, ret;
  
  /* Initialize resolver state pointer.  */
  //__resp = &pd->res;
  __resp = (void*) v__resp; //it seems that this was already initialized (default value?!)
  
  //__ctype_init (); // from glibc /* Initialize pointers to locale data.  */
  const uint16_t ** pctype_b = __ctype_b_loc ();
  const int32_t ** pctype_toupper = __ctype_toupper_loc ();
  const int32_t ** pctype_tolower = __ctype_tolower_loc ();

  if (pctype_b) *pctype_b = vctype_b;
  if (pctype_toupper) *pctype_toupper = vctype_toupper;
  if (pctype_tolower) *pctype_tolower = vctype_tolower;
  
#ifdef DEBUG_TLS
  printf("ctype_b %p{%p} ctype_toupper %p{%p} ctype_tolower %p{%p} ", 
	pctype_b, *pctype_b,
	pctype_toupper, *pctype_toupper, pctype_tolower, *pctype_tolower); 
  printf("errno %p{%d} __resp %p{%p}\n",
	 __errno_location(), *(int *)__errno_location(), &__resp, __resp);
#endif 
  
  /* actully run the user function */
  ret = (long)
    ((backend_args *)args)->cfunc(((backend_args *) args)->user);

  //__call_tls_dtors (); // from glibc
  //__nptl_deallocate_tsd (); // from glibc   /* Run the destructor for the thread-local data.  */

/* THE FOLLOWING HAS BEEN COMMENTED BECAUSE SOURCE OF OVERHEAD
#define CURRENT_STACK_FRAME \
 ({ register char *frame __asm__("rsp"); frame; })
  char * sp = CURRENT_STACK_FRAME;
  size_t freesize = (sp - (char*) ((backend_args *)args)->stackblock) & ~(4096 -1);
#define PTHREAD_STACK_MIN 16384
  if ( freesize > PTHREAD_STACK_MIN )
    madvise (((backend_args *)args)->stackblock, (freesize - PTHREAD_STACK_MIN), MADV_DONTNEED);
*/  
#ifdef DUMP_BACKEND    
  printf("%s: barg %p carg %p cfunc %p\n",
    __func__,  args, ((backend_args *)args)->cfunc,((backend_args *) args)->user);
     // for some reason (concurrency..) printf is using futex.. so avoid it in the current environment
#endif
  
//  free( args );

  // if to maintain here the following REMEMBER TO ADD DEFINES
   dump_current_tcb();
  dump_current_dtv();

  _ret = clone_exit(ret);
//  exit(ret);
  /* never reaching here */
  return (3333 + _ret);
}

// we assume stack is growing downward (tipical in x86)
//#define STACK_SIZE 0x1000 //have a look at the beginning of the program!
// from glibc
#define CLONE_SIGNAL            (CLONE_SIGHAND | CLONE_THREAD)

//int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
int cthread_create (cthread_t *thread, void* attr, void *(*cfunc)(void*), void *arg)
{
	int core_id = (int)((long) attr);
  int r = 0; // pthread_create(&pthread, NULL, cfunc, arg);
	#if GLIBC_STYLE
	  int clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGNAL
			     | CLONE_SETTLS | CLONE_PARENT_SETTID
			     //| CLONE_PARENT_SETTID
			     | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM
	#if __ASSUME_NO_CLONE_DETACHED == 0
/*For a while there was CLONE_DETACHED (introduced in 2.5.32): parent wants no child-exit  signal.   In
       2.6.2  the need to give this together with CLONE_THREAD disappeared.  This flag is still defined, but
       has no effect.
*/
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

// TODO liberate the memory before creating new threads
// TODO --- we can maintain a list ---
// TODO

//here: int err = ALLOCATE_STACK (iattr, &pd); (glibc/nptl/pthread_create.c)
#ifdef USE_MMAP_STACK
	    //copied from glibc/nptl/allocatestack.c
	    void * stack = mmap (0, STACK_SIZE, (PROT_READ | PROT_WRITE), MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	    
	    if ( (stack == MAP_FAILED) ) {
	      perror("mmap stack allocation error");
	      exit(0);
	    }
	    mprotect (stack, 4096, PROT_NONE);
#else
	    //void * stack = malloc(STACK_SIZE);
	    void * stack = calloc(STACK_SIZE, 1);
    	    if (!stack) {
		printf("stack allocation error");
		exit (0);
	    }
//	    memset(stack, 0, STACK_SIZE);
#endif	    

	    int memsz = 0; //memsz = phdr->p_memsz; <-- if there is a segment with phdr
	    int tcb_offset = memsz + roundup(_dl_tls_static_size, 1);
	    int _backend_size = tcb_offset + sizeof(struct backend) + TLS_INIT_TCB_ALIGN;	    
	    struct backend * __backendptr = calloc(_backend_size, 1) ;
	    if (!__backendptr) {
		printf("%s: TLS allocation error\n", __func__);
		exit (0);
	    }
	    //memset(__backendptr, 0, ( tcb_offset + sizeof(struct backend) + TLS_INIT_TCB_ALIGN)); //removed after using calloc instead of malloc
	#ifdef DEBUG_MALLOC_BACKEND
	    printf("malloc(tcb_offset %d, struct backend %ld, TLS_INIT_TCB %ld, total %ld) @ %p - %p\n",
		  tcb_offset, sizeof(struct backend), TLS_INIT_TCB_ALIGN,
		  (tcb_offset + sizeof(struct backend) + TLS_INIT_TCB_ALIGN), __backendptr,
		  ((char*)__backendptr + tcb_offset) );
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

/* ORIGINAL CODE --- DTV support /gnu/glibc/elf/dl-tls.c
void * internal_function _dl_allocate_tls (void *mem)
{
  return _dl_allocate_tls_init (mem == NULL
				? _dl_allocate_tls_storage ()
				: allocate_dtv (mem));
}*/	    
	    void* retptr = allocate_dtv(backendptr);
	    if (retptr == NULL) {
	      printf("allocation of dtv failed for cpu %d, exit\n", core_id);
	      exit(0);
	    }
	    if (_dl_allocate_tls_init (retptr) == NULL) {
	      printf("allocation of tls failed for cpu %d, exit\n", core_id);
	      exit(0);
	    }

	    backend_args * bkargs = calloc(sizeof(backend_args), 1);
	    if (!bkargs) {
		printf("args allocation error\n");
		exit (0);
	    }
	    bkargs->user = arg;
	    bkargs->stackblock = stack;
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
		exit (0);
	    }
#ifdef DUMP_BACKEND
	    printf ("%s: TID %d, cpuid %d stack %p tls %p barg %p carg %p bfunc %p cfunc %p\n",
		    __func__, r, core_id,
		    (stack+STACK_SIZE), backendptr,
		    bkargs, arg, backend_start_func, cfunc);
#endif

// TODO	    DO I HAVE TO SAVE _backend_args here?!

return r;

}

// TLS
/*antoniob@gigi:/home/antoniob/mklinux-threads/mklinux-utils-cthread/bomp/lib$ readelf -s /lib/libc-2.11.1.so | grep TLS
   291: 0000000000000010     4 TLS     GLOBAL DEFAULT   22 errno@@GLIBC_PRIVATE
  1434: 0000000000000054     4 TLS     GLOBAL DEFAULT   22 h_errno@@GLIBC_PRIVATE
  1824: 0000000000000008     8 TLS     GLOBAL DEFAULT   21 __resp@@GLIBC_PRIVATE

TLS_START: 0x6047a8:       0x00007ffff7dd7580  __resp: 0x00007ffff7ddb300
0x6047b8: errno: 0x00000000 00000000    tolower: 0x00007ffff7b8d180
0x6047c8: toupper: 0x00007ffff7b8d980    ctype: 0x00007ffff7b8df80
0x6047d8:       0x00007ffff7dd8e40      0x0000000000000000
0x6047e8:       0x0000000000000000      0x0000000000000000
0x6047f8:       0x00000000 h_errno: 00000000      0x0000000000000000
0x604808:       0x0000000000000000      
TCB_START:                                         0x0000000000604810
0x604818:       0x0000000000604d30      0x0000000000604810
0x604828:       0x0000000000000001      0x0000000000000000
0x604838:       0x468ecd5300cd7400      0x8edfaac970a9a4cb
*/
