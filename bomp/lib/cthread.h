
/*
 * cthread library header
 *
 * This library is a kind of hack in order to make application developed with
 * glibc to be able to use threads without adopting pthread lib (but cthread)
 * pthread is heavily based on futex, cthread does not use futex. Warning,
 * printf uses futex! Avoid printf in your code.
 *
 * Copyright Antonio Barbalace, SSRG, VT, 2013
 */

/* Thread identifiers.  The structure of the attribute type is not
   exposed on purpose.  */
typedef unsigned long int cthread_t;

/* Keys for thread-specific data */
typedef unsigned int cthread_key_t;




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
typedef struct _cpu_set_t
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

int cthread_setaffinity_np(pid_t pid, size_t cpusetsize, const cpu_set_t *cpuset)  ; // NOTE first argument is not pthread or cthread
//int cthread_getaffinity_np(pthread_t thread, size_t cpusetsize, cpu_set_t *cpuset); // TODO





//returns the previous selector
unsigned long cthread_initialize (void );
void cthread_restore (unsigned long selector);

int cthread_create (cthread_t *thread, void* attr, void *(*start_routine)(void*), void *arg);

//void pthread_exit(void *value_ptr); // a correspondent pthread exists but the idea is to terminate the thread before the normal exit (differently from here)
//cthread_exit(ret); // this must be hidden inside the thread return function

int cthread_key_create (cthread_key_t *key, void (*destr) (void *));
void * cthread_getspecific(cthread_key_t key);
int cthread_setspecific (cthread_key_t key, const void *value);

cthread_t cthread_self (void );
