
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

//returns the previous selector
unsigned long cthread_initialize (void );

static inline void cthread_restore (unsigned long selector)
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

int cthread_create (cthread_t *thread, void* attr, void *(*start_routine)(void*), void *arg);

//void pthread_exit(void *value_ptr); // a correspondent pthread exists but the idea is to terminate the thread before the normal exit (differently from here)
//cthread_exit(ret); // this must be hidden inside the thread return function

int cthread_setaffinity_np(pid_t pid, size_t cpusetsize, const cpu_set_t *cpuset); // NOTE first argument is not pthread or cthread
//int cthread_getaffinity_np(pthread_t thread, size_t cpusetsize, cpu_set_t *cpuset); // TODO

int cthread_key_create (backend_key_t *key, void (*destr) (void *));
void * cthread_getspecific(backend_key_t key);
int cthread_setspecific (backend_key_t key, const void *value);

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

static inline struct backend * cthread_self (void ) {
	return THREAD_SELF; }


