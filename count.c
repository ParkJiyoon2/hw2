#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

int global_counter = 0;
pthread_mutex_t lock;
pthread_mutex_t lock2;

// thread_inc
static void * thread_inc(void *arg)
{
	int i;
	for (int i = 0 ; i < 10000000 ; i++) {
		pthread_mutex_lock(&lock);
		global_counter++;
		pthread_mutex_unlock(&lock);
	}
	// thread exit
	pthread_exit(0);
}

// thread_dec
static void * thread_dec(void *arg)
{
	int i;
	for (int i = 0 ; i < 10000000 ; i++) {
		pthread_mutex_lock(&lock);
		global_counter--;
		pthread_mutex_unlock(&lock);
	}
	// thread exit
	pthread_exit(0);
}


int main(int argc, char *argv[])
{

	pthread_t thr[4];
	void *res;
	struct timeval start, end;

	// mutex init
	pthread_mutex_init(&lock, NULL);
	pthread_mutex_init(&lock2, NULL);

	// print global_counter;
	printf("global_counter: %d\n", global_counter);

	//gettimeofday for getting start time
	gettimeofday(&start, NULL);

	//create two threads
	pthread_create(&thr[0], NULL, thread_inc, NULL);
	pthread_create(&thr[1], NULL, thread_dec, NULL);


	pthread_join(thr[0], &res);
	pthread_join(thr[1], &res);

	//gettimeofday for getting end time
	gettimeofday(&end, NULL);

	// print global_counter;
	printf("global_counter: %d  time: %ld:%ld\n", global_counter, 
			end.tv_sec - start.tv_sec, end.tv_usec - start.tv_usec );

	global_counter = 0;
	// print global_counter;
	printf("reset counter\nglobal_counter: %d\n", global_counter);

	//create two threads
	pthread_create(&thr[2], NULL, thread_inc, NULL);
	pthread_create(&thr[3], NULL, thread_inc, NULL);

	pthread_join(thr[2], &res);
	pthread_join(thr[3], &res);

	// print global_counter;
	printf("global_counter: %d\n", global_counter);

	return 0;
}
