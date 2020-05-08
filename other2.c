#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

typedef struct sharedobject {
	FILE *rfile;
	int linenum;
	char *line;
	pthread_mutex_t lock;
	pthread_cond_t cv;
	int full;
} so_t;

void *producer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		// 개행할때까지 읽어들임
		// end of file이면 -1 반환
		read = getdelim(&line, &len, '\n', rfile);

		pthread_mutex_lock(&so->lock);

		// 처음 실행시 false
		// 그후 다시 producer실행했으면 밑에서full=1해줬기 떄문에 consumer먼저 실행되도록 wait
		 while(so->full == 1)
                {
                        // consumer가 깨우도록함
                        pthread_cond_wait(&so->cv, &so->lock);
                }


		if (read == -1) { // eof condition
			so->full = 1;
			so->line = NULL;

			pthread_cond_signal(&so->cv);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		// buffer 비어있으면 shared buffer에 line 올림
		
		// buffer full, lock
		so->linenum = i;
		so->line = strdup(line);      /* share the line */
		i++;

		// producer job completed
		so->full = 1;

		pthread_cond_signal(&so->cv);

		pthread_mutex_unlock(&so->lock);
	}
	free(line);
	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}

void *consumer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	int len;
	char *line;

	while (1) {
		// shared access, lock
		pthread_mutex_lock(&so->lock);

		//consumer는 buffer가 비어있으면 동작불가
		//consumer waits until buffer is full
		while(so->full == 0) 
		{
			pthread_cond_wait(&so->cv, &so->lock);
		}

		//now, buffer is full
		//mutex lock is hold

		line = so->line; // shared line 읽고
		if (line == NULL) {
			pthread_cond_signal(&so->cv);
			pthread_mutex_unlock(&so->lock);
			break; // 비었으면 break
		}
		//len = strlen(line);
		printf("Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->linenum, line);
		free(so->line);
		i++;
		
		//consumer job complete

		so->full = 0;
		pthread_cond_signal(&so->cv);
		pthread_mutex_unlock(&so->lock);
	}
	printf("Cons: %d lines\n", i);
	*ret = i;
	pthread_exit(ret);
}


int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;
	int rc;   long t;
	int *ret;
	int i;
	FILE *rfile;

	// time val
	struct timeval start, end;

	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}
	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));
	rfile = fopen((char *) argv[1], "r");
	if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}
	if (argv[2] != NULL) {
		Nprod = atoi(argv[2]);
		if (Nprod > 100) Nprod = 100;
		if (Nprod == 0) Nprod = 1;
	} else Nprod = 1;
	if (argv[3] != NULL) {
		Ncons = atoi(argv[3]);
		if (Ncons > 100) Ncons = 100;
		if (Ncons == 0) Ncons = 1;
	} else Ncons = 1;
	
	share->rfile = rfile;
	share->line = NULL;
	pthread_mutex_init(&share->lock, NULL);
	pthread_cond_init(&share->cv, NULL);
	
	// time
	gettimeofday(&start, NULL);
	
	
		pthread_create(&prod[0], NULL, producer, share);
	
		pthread_create(&cons[0], NULL, consumer, share);
	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}

	// time
	gettimeofday(&end, NULL);
	printf("due time : %ld:%ld\n", end.tv_sec - start.tv_sec, end.tv_usec - start.tv_usec);

	pthread_exit(NULL);
	exit(0);
}
