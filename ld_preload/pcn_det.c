/*
 * Copyright (C) 2016 Yuzhong Wen <wyz2014@vt.edu>
 *
 * Deterministic system library interception
 */

#include <sys/syscall.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#define _GNU_SOURCE
#include <dlfcn.h>

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    int ret;
    void *handle;
    static int (*pthread_mutex_lock_real)(pthread_mutex_t *mutex) = NULL;
    handle = dlopen("/lib/x86_64-linux-gnu/libc.so.6", RTLD_LAZY);
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
    handle = dlopen("/lib/x86_64-linux-gnu/libc.so.6", RTLD_LAZY);
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
    handle = dlopen("/lib/x86_64-linux-gnu/libc.so.6", RTLD_LAZY);
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
    handle = dlopen("/lib/x86_64-linux-gnu/libc.so.6", RTLD_LAZY);
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
    handle = dlopen("/lib/x86_64-linux-gnu/libc.so.6", RTLD_LAZY);
    if (!pthread_rwlock_trywrlock_real)
        pthread_rwlock_trywrlock_real = dlsym(handle, "pthread_rwlock_trywrlock");

    syscall(319);
    ret = pthread_rwlock_trywrlock_real(rwlock);
    syscall(320);

    return ret;
}
