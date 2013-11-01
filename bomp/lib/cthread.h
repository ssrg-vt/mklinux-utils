
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

//returns the previous selector
unsigned long cthread_initialize (void );
void cthread_restore (unsigned long selector);

int cthread_create (cthread_t *thread, void* attr, void *(*start_routine)(void*), void *arg);

//void pthread_exit(void *value_ptr); // a correspondent pthread exists but the idea is to terminate the thread before the normal exit (differently from here)
//cthread_exit(ret); // this must be hidden inside the thread return function

int cthread_setaffinity_np(pid_t pid, size_t cpusetsize, const cpu_set_t *cpuset); // NOTE first argument is not pthread or cthread
//int cthread_getaffinity_np(pthread_t thread, size_t cpusetsize, cpu_set_t *cpuset); // TODO

int cthread_key_create (cthread_key_t *key, void (*destr) (void *));
void * cthread_getspecific(cthread_key_t key);
int cthread_setspecific (cthread_key_t key, const void *value);

cthread_t cthread_self (void );

