/*
 * pthread_test.c
 * Copyright (C) 2016 wen <wen@Saturn>
 *
 * Distributed under terms of the MIT license.
 */

#include <pthread.h>

pthread_mutex_t mtx;
pthread_cond_t cv;

void *lel(void *data)
{
	pthread_cond_wait(&cv, &mtx);
}

int main()
{
//	pthread_mutex_lock(&mtx);
//	pthread_mutex_unlock(&mtx);
	pthread_t thread[10];
	int i = 0;
	for (i = 0; i < 10; i ++) {
		pthread_create(&thread, NULL, lel, NULL);
	}
	sleep(5);
	for (i = 0; i < 10; i ++) {
		pthread_cond_signal(&cv);
	}
	pthread_cond_init(&cv, NULL);
	return 0;
}
