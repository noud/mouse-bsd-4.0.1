/*	$NetBSD: cond3.c,v 1.1 2003/01/30 18:53:47 thorpej Exp $	*/

#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *threadfunc(void *arg);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int
main(int argc, char *argv[])
{
	int x,ret;
	pthread_t new;
	void *joinval;
	int sharedval;

	printf("1: condition variable test 3\n");

	x = 20;
	pthread_mutex_lock(&mutex);

	sharedval = 1;

	ret = pthread_create(&new, NULL, threadfunc, &sharedval);
	if (ret != 0)
		err(1, "pthread_create");

	printf("1: Before waiting.\n");
	do {
		sleep(2);
		pthread_cond_wait(&cond, &mutex);
		printf("1: After waiting, in loop.\n");
	} while (sharedval != 0);

	printf("1: After the loop.\n");

	pthread_mutex_unlock(&mutex);

	printf("1: After releasing the mutex.\n");
	ret = pthread_join(new, &joinval);
	if (ret != 0)
		err(1, "pthread_join");

	printf("1: Thread joined.\n");

	return 0;
}

void *
threadfunc(void *arg)
{
	int *share = (int *) arg;

	printf("2: Second thread.\n");

	printf("2: Locking mutex\n");
	pthread_mutex_lock(&mutex);
	printf("2: Got mutex.\n");
	printf("Shared value: %d. Changing to 0.\n", *share);
	*share = 0;
	
	/* Signal first, then unlock, for a different test than #1. */
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

	return NULL;
}
