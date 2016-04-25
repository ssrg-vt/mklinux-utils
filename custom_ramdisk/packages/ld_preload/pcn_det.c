/*
 * Copyright (C) 2016 Yuzhong Wen <wyz2014@vt.edu>
 *
 * Deterministic system library interception
 * For cond_wait, 99% of the code is from glibc 2.15
 */

#include <sys/syscall.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#define _GNU_SOURCE
#include <dlfcn.h>

#define PTHREAD_PATH "/lib/x86_64-linux-gnu/libpthread.so.0"

typedef int8_t atomic8_t;
typedef uint8_t uatomic8_t;
typedef int_fast8_t atomic_fast8_t;
typedef uint_fast8_t uatomic_fast8_t;

typedef int16_t atomic16_t;
typedef uint16_t uatomic16_t;
typedef int_fast16_t atomic_fast16_t;
typedef uint_fast16_t uatomic_fast16_t;

typedef int32_t atomic32_t;
typedef uint32_t uatomic32_t;
typedef int_fast32_t atomic_fast32_t;
typedef uint_fast32_t uatomic_fast32_t;

typedef int64_t atomic64_t;
typedef uint64_t uatomic64_t;
typedef int_fast64_t atomic_fast64_t;
typedef uint_fast64_t uatomic_fast64_t;

typedef intptr_t atomicptr_t;
typedef uintptr_t uatomicptr_t;
typedef intmax_t atomic_max_t;
typedef uintmax_t uatomic_max_t;


#define FUTEX_WAIT  0
#define FUTEX_WAKE  1
#define FUTEX_FD  2
#define FUTEX_REQUEUE  3
#define FUTEX_CMP_REQUEUE 4
#define FUTEX_WAKE_OP  5
#define FUTEX_LOCK_PI  6
#define FUTEX_UNLOCK_PI 7
#define FUTEX_TRYLOCK_PI 8
#define FUTEX_WAIT_BITSET 9
#define FUTEX_WAKE_BITSET 10
#define FUTEX_WAIT_REQUEUE_PI 11
#define FUTEX_CMP_REQUEUE_PI 12

#define FUTEX_PRIVATE_FLAG 128
#define FUTEX_CLOCK_REALTIME 256
#define FUTEX_CMD_MASK  ~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME)

#define FUTEX_WAKE_OP_PRIVATE (FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG)

#define FUTEX_WAIT_PRIVATE (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_PRIVATE (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)

#define FUTEX_OP_CLEAR_WAKE_IF_GT_ONE ((4 << 24) | 1)

#define COND_NWAITERS_SHIFT 1
#define LLL_PRIVATE 128
#define LLL_SHARED 0

#define atomic_compare_and_exchange_bool_acq(mem, newval, oldval) \
      (! __sync_bool_compare_and_swap (mem, oldval, newval))

# define atomic_exchange_rel(mem, newvalue) atomic_exchange_acq (mem, newvalue)

/* Note that we need no lock prefix.  */
#define atomic_exchange_acq(mem, newvalue) \
  ({ __typeof (*mem) result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("xchgb %b0, %1"				      \
			 : "=q" (result), "=m" (*mem)		      \
			 : "0" (newvalue), "m" (*mem));		      \
     else if (sizeof (*mem) == 2)				      \
       __asm __volatile ("xchgw %w0, %1"				      \
			 : "=r" (result), "=m" (*mem)		      \
			 : "0" (newvalue), "m" (*mem));		      \
     else if (sizeof (*mem) == 4)				      \
       __asm __volatile ("xchgl %0, %1"				      \
			 : "=r" (result), "=m" (*mem)		      \
			 : "0" (newvalue), "m" (*mem));		      \
     else							      \
       __asm __volatile ("xchgq %q0, %1"				      \
			 : "=r" (result), "=m" (*mem)		      \
			 : "0" ((atomic64_t) newvalue),     \
			   "m" (*mem));				      \
     result; })

#define lll_futex_wait(futex, val, private)     \
        syscall(202, futex, FUTEX_WAIT_PRIVATE, val, NULL, NULL, 0)

#define lll_futex_wake(futex, nr, private)                             \
        syscall(202, futex, FUTEX_WAKE_PRIVATE, nr, NULL, NULL, 0)

#define lll_futex_wake_unlock(futex, nr_wake, nr_wake2, futex2, private) \
        syscall(202, futex, FUTEX_WAKE_OP_PRIVATE, nr_wake, nr_wake2, futex2, \
                FUTEX_OP_CLEAR_WAKE_IF_GT_ONE)

#define __lll_lock(futex, private)                                      \
  ((void)                                                               \
   ({                                                                   \
     int *__futex = (futex);                                            \
       if (atomic_compare_and_exchange_bool_acq (__futex, 1, 0))        \
       {                                                                \
         if (__builtin_constant_p (private) && (private) == LLL_PRIVATE) \
           __lll_lock_wait_private (__futex);                           \
         else                                                           \
           __lll_lock_wait (__futex, private);                          \
       }                                                                \
   }))
#define lll_lock(futex, private) \
  __lll_lock (&(futex), private)

#define __lll_unlock(futex, private)                    \
  ((void)                                               \
   ({                                                   \
     int *__futex = (futex);                            \
     int __private = (private);                         \
     int __oldval = atomic_exchange_rel (__futex, 0);   \
     if (__oldval > 1)               \
       lll_futex_wake (__futex, 1, __private);          \
   }))
#define lll_unlock(futex, private)\
  __lll_unlock (&(futex), private)

void
__lll_lock_wait_private (int *futex)
{
  if (*futex == 2)
    lll_futex_wait (futex, 2, LLL_PRIVATE); /* Wait if *futex == 2.  */

  while (atomic_exchange_acq (futex, 2) != 0)
    lll_futex_wait (futex, 2, LLL_PRIVATE); /* Wait if *futex == 2.  */
}

void
__lll_lock_wait (int *futex, int private)
{
  if (*futex == 2)
    lll_futex_wait (futex, 2, private); /* Wait if *futex == 2.  */

  while (atomic_exchange_acq (futex, 2) != 0)
    lll_futex_wait (futex, 2, private); /* Wait if *futex == 2.  */
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    int pshared = LLL_PRIVATE;
    syscall(319);
    lll_lock(cond->__data.__lock, pshared);

    if (cond->__data.__total_seq > cond->__data.__wakeup_seq) {
        ++cond->__data.__wakeup_seq;
        ++cond->__data.__futex;

        if (!__builtin_expect (lll_futex_wake_unlock (&cond->__data.__futex, 1, 1, &cond->__data.__lock, pshared), 0)) {
            syscall(320);
            return 0;
        }

        lll_futex_wake(&cond->__data.__futex, 1, pshared);
    }

    lll_unlock(cond->__data.__lock, pshared);
    syscall(320);

    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    int err;
    int pshared = LLL_PRIVATE;
    // We're gonna have our own compact implementation of cond_wait so that we won't
    // miss the lock acquisitions inside pthread_cond_wait.
    // Here we are gonna support only basic cond_wait. PI futex, pthread_cancel, shared
    // futex are not supportted

    syscall(319);
    lll_lock(cond->__data.__lock, pshared);
    syscall(320);
    // pthread_mutex_unlock will do --mutex->__data.__nusers, however for the unlock
    // in cond_wait should not do this, so we add it by one first.
    ++mutex->__data.__nusers;
    err = pthread_mutex_unlock(mutex);
    if (__builtin_expect(err, 0)) {
        lll_unlock(cond->__data.__lock, pshared);
        return err;
    }

    syscall(319);
    ++cond->__data.__total_seq;
    ++cond->__data.__futex;
    syscall(320);
    cond->__data.__nwaiters += 1 << COND_NWAITERS_SHIFT;

    if (cond->__data.__mutex != (void *) ~0l)
        cond->__data.__mutex = mutex;

    unsigned long long int val;
    unsigned long long int seq;
    unsigned long long int bc_seq;
    val = seq = cond->__data.__wakeup_seq;
    bc_seq = cond->__data.__broadcast_seq;

    do {
        unsigned int futex_val = cond->__data.__futex;
        lll_unlock(cond->__data.__lock, pshared);
        syscall(321, 0);
        lll_futex_wait(&cond->__data.__futex, futex_val, pshared);
        syscall(319);
        lll_lock(cond->__data.__lock, pshared);
        syscall(320);

        if (bc_seq != cond->__data.__broadcast_seq)
            goto bc_out;
        val = cond->__data.__wakeup_seq;
    } while (val == seq || cond->__data.__woken_seq == val);

    ++cond->__data.__woken_seq;
bc_out:
    cond->__data.__nwaiters -= 1 << COND_NWAITERS_SHIFT;

    lll_unlock(cond->__data.__lock, pshared);

    syscall(319);
    err = pthread_mutex_lock(mutex);
    syscall(320);
    return err;
}

/*
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    int ret;
    void *handle;
    static int (*pthread_mutex_lock_real)(pthread_mutex_t *mutex) = NULL;
    handle = dlopen(PTHREAD_PATH, RTLD_LAZY);
    if (!handle) {
        printf("cannot open!\n");
    }
    if (!pthread_mutex_lock_real)
        pthread_mutex_lock_real = dlsym(handle, "pthread_mutex_lock");

    syscall(319);
    ret = pthread_mutex_lock_real(mutex);
    syscall(320);

    return ret;
}


int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    int ret;
    void *handle;
    static int (*pthread_mutex_trylock_real)(pthread_mutex_t *mutex) = NULL;
    int (*pthread_mutex_lock_real)(pthread_mutex_t *mutex) = NULL;
    handle = dlopen(PTHREAD_PATH, RTLD_LAZY);
    if (!pthread_mutex_trylock_real)
        pthread_mutex_trylock_real = dlsym(handle, "pthread_mutex_trylock");

    syscall(319);
    ret = pthread_mutex_trylock_real(mutex);
    syscall(320);

    return ret;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
    int ret;
    void *handle;
    static int (*pthread_rwlock_rdlock_real)(pthread_rwlock_t *rwlock) = NULL;
    int (*pthread_mutex_lock_real)(pthread_mutex_t *mutex) = NULL;
    handle = dlopen(PTHREAD_PATH, RTLD_LAZY);
    if (!pthread_rwlock_rdlock_real)
        pthread_rwlock_rdlock_real = dlsym(handle, "pthread_rwlock_rdlock");

    syscall(319);
    ret = pthread_rwlock_rdlock_real(rwlock);
    syscall(320);

    return ret;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
    int ret;
    void *handle;
    static int (*pthread_rwlock_tryrdlock_real)(pthread_rwlock_t *rwlock) = NULL;
    int (*pthread_mutex_lock_real)(pthread_mutex_t *mutex) = NULL;
    handle = dlopen(PTHREAD_PATH, RTLD_LAZY);
    if (!pthread_rwlock_tryrdlock_real)
        pthread_rwlock_tryrdlock_real = dlsym(handle, "pthread_rwlock_tryrdlock");

    syscall(319);
    ret = pthread_rwlock_tryrdlock_real(rwlock);
    syscall(320);

    return ret;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
    int ret;
    void *handle;
    static int (*pthread_rwlock_wrlock_real)(pthread_rwlock_t *rwlock) = NULL;
    if (!pthread_rwlock_wrlock_real)
        pthread_rwlock_wrlock_real = dlsym(handle, "pthread_rwlock_wrlock");

    syscall(319);
    ret = pthread_rwlock_wrlock_real(rwlock);
    syscall(320);

    return ret;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
    int ret;
    void *handle;
    static int (*pthread_rwlock_trywrlock_real)(pthread_rwlock_t *rwlock) = NULL;
    int (*pthread_mutex_lock_real)(pthread_mutex_t *mutex) = NULL;
    handle = dlopen(PTHREAD_PATH, RTLD_LAZY);
    if (!pthread_rwlock_trywrlock_real)
        pthread_rwlock_trywrlock_real = dlsym(handle, "pthread_rwlock_trywrlock");

    syscall(319);
    ret = pthread_rwlock_trywrlock_real(rwlock);
    syscall(320);

    return ret;
}
*/
