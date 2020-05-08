#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct sharedobject {
	FILE *rfile;
	int linenum;
	char *line[100];   //main 함수에서 쓰레드로 쓸 수 있는 갯수의 총계가 100;
	pthread_mutex_t lock;
	pthread_cond_t cv;
	int full;
	int buf_full[100];
	int num_of_empty;    //empty된 갯수
	int C_num;   //선언된 Consumer의 갯수
} so_t;


int C_index;
int P_last;

void *producer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;
	int target = 0;


	while (1) {
		read = getdelim(&line, &len, '\n', rfile);
		pthread_mutex_lock(&so->lock);
		while(so->full == 1){
			pthread_cond_wait(&so->cv, &so->lock);
		}

		if (read == -1) {
			so->full = 1;
			for(int i = 0; i < C_index; i++ ){
				so->buf_full[i] = 1;
			}
			so->line[i] = NULL;
			P_last = 1;

			pthread_cond_broadcast(&so->cv);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		so->linenum = i;
		so->line[target] = strdup(line);      /* share the line */
		i++;
		so->buf_full[target] = 1;
		target++;
		if(target == so->C_num){
			target = 0;
			so->full = 1;
			pthread_cond_broadcast(&so->cv);
		}
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
	int target = 0;


	pthread_mutex_lock(&so->lock);
	target = C_index++;
      	printf("C_index=%d target=%d\n",C_index,target);
		//버퍼 지정
	printf("TARGET BUF %x : %d\n", (unsigned int)pthread_self(), target);  //버퍼 지정 잘 됐나 확인.
	pthread_mutex_unlock(&so->lock);

	while (1) {
		pthread_mutex_lock(&so->lock);
		line = so->line[target];
		while(line == NULL && so->buf_full[target] == 0){
			pthread_cond_wait(&so->cv, &so->lock);
		}

		line = so->line[target];
		if (line == NULL) {
			pthread_cond_broadcast(&so->cv);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		len = strlen(line);
		printf("Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->linenum, line);
		free(so->line[target]);
		so->line[target] = NULL;
		i++;
		so->buf_full[target] = 0;
		so->num_of_empty++;

		if(so->num_of_empty == so->C_num)
		{
			so->num_of_empty = 0;
			so->full = 0;
		}

		if(P_last == 1){
			pthread_cond_broadcast(&so->cv);
			pthread_mutex_unlock(&so->lock);
			break;
		}


		pthread_cond_broadcast(&so->cv);
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

	share->C_num = Ncons;

	share->rfile = rfile;
	pthread_mutex_init(&share->lock, NULL);
	pthread_cond_init(&share->cv, NULL);
	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);
	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}
	pthread_exit(NULL);
	exit(0);
}
