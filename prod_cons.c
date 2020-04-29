#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct sharedobject {
	FILE *rfile;
	int linenum;
	char *line;
	pthread_mutex_t lock;
	int full;
	pthread_cond_t cond;
} so_t;

void *producer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0; //줄 수
	char *line = NULL; //파일 내용 문자열 받는 변수
	size_t len = 0;
	ssize_t read = 0;

	pthread_mutex_lock(&so->lock);	
	
	while (1) {
		read = getdelim(&line, &len, '\n', rfile);
		if (read == -1) {
			so->full = 1;
			//so->line = NULL;
				
			break;
		}
		so->linenum = i;
		//so->line = strdup(line);      /* share the line */
		strcat(so->line, line);

		i++;
		so->full = 1;
	}
	
	so->linenum=i; //줄 수동기화

	free(line);
	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	
	*ret = i;
	
	pthread_mutex_unlock(&so->lock);
	pthread_cond_signal(&so->cond);
	pthread_exit(ret);

}

void *consumer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	int len;
	char *line;
	
	pthread_mutex_lock(&so->lock);

	line=so->line;
	
	len=strlen(line);

	int save=0;
	
	while (1) {
		if (line == NULL || save>=len) {
			break;
		}
		printf("Cons_%x: [%02d:%02d] ", (unsigned int)pthread_self(), i+1, so->linenum);

		for(int o=save; o<len; o++){
			printf("%c", line[o]);
			if(line[o]=='\n' || line[o]==NULL){
				save=o+1;
				i++;
				break;
			}
		}

		//printf("Cons_%x: [%02d:%02d] %s",
		//	(unsigned int)pthread_self(), i, so->linenum, line);
		//free(so->line);		
		so->full = 0;
	}
	
	//free(so->line);
	printf("Cons: %d lines\n", i);
	
	*ret = i;

	pthread_mutex_unlock(&so->lock);
	pthread_cond_signal(&so->cond);
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
	if (argc == 1) { //인수가 없으면
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
		printf("들어옴");
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
	share->line = malloc(3000);
	pthread_mutex_init(&share->lock, NULL);
	
	
	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	
	
	pthread_cond_wait(&share->cond, &share->lock);
	
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);
	
	pthread_cond_wait(&share->cond, &share->lock);
	printf("main continuing\n");
	
	
	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}
	
	pthread_exit(ret);
	exit(0);
}

